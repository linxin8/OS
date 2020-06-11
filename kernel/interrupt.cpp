#include "kernel/interrupt.h"
#include "kernel/asm_interface.h"
#include "kernel/boot_config.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "thread/thread.h"

#define PIC_M_CTRL 0x20  // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21  // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0  // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1  // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x81  // 目前总共支持的中断数

#define EFLAGS_IF 0x00000200  // eflags寄存器中的if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g"(EFLAG_VAR))

extern "C" {
//  参考interrupt.asm
extern uint32_t syscall_handler(uint32_t);
//  参考interrupt.asm
extern InterruptHandler interrupt_entry_table[IDT_DESC_CNT];
};

// 中断门描述符结构体,x86固定结构
struct GateDescript
{
    uint16_t function_offset_low_word;
    uint16_t selector;
    uint8_t  dcount;  //此项为双字计数字段，是门描述符中的第4字节。此项固定值，不用考虑
    uint8_t  attribute;
    uint16_t function_offset_high_word;
    GateDescript() = default;
    GateDescript(uint8_t attribute, InterruptHandler function)
        : function_offset_low_word((uint32_t)function & 0x0000FFFF), selector(SELECTOR_K_CODE), dcount(0),
          attribute(attribute), function_offset_high_word(((uint32_t)function & 0xFFFF0000) >> 16)
    {
    }
};

GateDescript idt[IDT_DESC_CNT];  // idt是中断描述符表,本质上就是个中断门描述符数组

const char* interrupt_name[IDT_DESC_CNT];  // 用于保存异常的名字

//  中断处理函数，参考interrupt.asm
InterruptHandler idt_table[IDT_DESC_CNT];

/* 初始化可编程中断控制器8259A */
void init_pic()
{
    /* 初始化主片 */
    outb(PIC_M_CTRL, 0x11);  // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_M_DATA, 0x20);  // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outb(PIC_M_DATA, 0x04);  // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0x01);  // ICW4: 8086模式, 正常EOI

    /* 初始化从片 */
    outb(PIC_S_CTRL, 0x11);  // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_S_DATA, 0x28);  // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
    outb(PIC_S_DATA, 0x02);  // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01);  // ICW4: 8086模式, 正常EOI

    /* IRQ2用于级联从片,必须打开,否则无法响应从片上的中断
    主片上打开的中断有IRQ0的时钟,IRQ1的键盘和级联从片的IRQ2,其它全部关闭 */
    outb(PIC_M_DATA, 0xf8);
    /* 打开从片上的IRQ14,此引脚接收硬盘控制器的中断 */
    outb(PIC_S_DATA, 0xbf);

    put_str("   pic_init done\n");
}

/*初始化中断描述符表*/
void init_idt_descript(void)
{
    for (int i = 0; i < IDT_DESC_CNT; i++)
    {
        idt[i] = GateDescript(IDT_DESC_ATTR_DPL0, interrupt_entry_table[i]);
    }
    /* 单独处理系统调用,系统调用对应的中断门dpl为3,
     * 中断处理程序为单独的syscall_handler */
    idt[0x80] = GateDescript(IDT_DESC_ATTR_DPL3, (void*)syscall_handler);
    printkln("   idt_desc_init done");
}

/* 默认的中断处理函数，直接报错*/
void default_interrupt_handler(uint32_t no)
{
    if (no == 0x27 || no == 0x2f)
    {            // 0x2f是从片8259A上的最后一个irq引脚，保留
        return;  // IRQ7和IRQ15会产生伪中断(spurious interrupt),无须处理。
    }
    printkln("interrupt excetion no: %d, name: %s, pid: %d, user: %s", no, interrupt_name[no],
             Thread::get_current_pcb()->pid, Thread::get_current_pcb()->name);
    if (no == 14)
    {  // 若为Pagefault,将缺失的地址打印出来并悬停
        uint32_t page_fault_vaddr = 0;
        asm("movl %%cr2, %0" : "=r"(page_fault_vaddr));  // cr2是存放造成page_fault的地址
        printkln("page fault addr is %x", page_fault_vaddr);
    }
    // 能进入中断处理程序就表示已经处在关中断情况下,
    // 不会出现调度进程的情况。故下面的死循环不会再被中断。
    Debug::hlt();
}

/* 完成一般中断处理函数注册及异常名称注册 */
void init_idt_table()
{  // 完成一般中断处理函数注册及异常名称注册
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        /* idt_table数组中的函数是在进入中断后根据中断向量号调用的,
         * 见kernel/kernel.S的call [idt_table + %1*4] */
        idt_table[i] = (void*)default_interrupt_handler;  // 默认为default_interrupt_handler
                                                          // 以后会由register_handler来注册具体处理函数。
        interrupt_name[i] = "unknown";                    // 先统一赋值为unknown
    }
    interrupt_name[0]  = "#DE Divide Error";
    interrupt_name[1]  = "#DB Debug Exception";
    interrupt_name[2]  = "NMI Interrupt";
    interrupt_name[3]  = "#BP Breakpoint Exception";
    interrupt_name[4]  = "#OF Overflow Exception";
    interrupt_name[5]  = "#BR BOUND Range Exceeded Exception";
    interrupt_name[6]  = "#UD Invalid Opcode Exception";
    interrupt_name[7]  = "#NM Device Not Available Exception";
    interrupt_name[8]  = "#DF Double Fault Exception";
    interrupt_name[9]  = "Coprocessor Segment Overrun";
    interrupt_name[10] = "#TS Invalid TSS Exception";
    interrupt_name[11] = "#NP Segment Not Present";
    interrupt_name[12] = "#SS Stack Fault Exception";
    interrupt_name[13] = "#GP General Protection Exception";
    interrupt_name[14] = "#PF Page-Fault Exception";
    // interrupt_name[15] 第15项是intel保留项，未使用
    interrupt_name[16] = "#MF x87 FPU Floating-Point Error";
    interrupt_name[17] = "#AC Alignment Check Exception";
    interrupt_name[18] = "#MC Machine-Check Exception";
    interrupt_name[19] = "#XF SIMD Floating-Point Exception";
}

/* 开中断并返回之前的中断状态*/
void Interrupt::enable()
{
    asm volatile("sti");  // 开中断,sti指令将IF位置1
}

// 关中断并返回之前的中断状态
void Interrupt::disable()
{
    asm volatile("cli" : : : "memory");  // 关中断,cli指令将IF位置0
}

// 将中断状态设置为status
void Interrupt::set_status(InterruptStatus status)
{
    status == InterruptStatus::on ? Interrupt::enable() : Interrupt::disable();
}

// 获取当前中断状态
InterruptStatus Interrupt::get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? InterruptStatus::on : InterruptStatus::off;
}

// 设置中断号为no的中断处理函数
void Interrupt::register_interrupt_handler(uint8_t no, InterruptHandler function)
{
    idt_table[no] = function;
}

bool Interrupt::is_enabled()
{
    return get_status() == InterruptStatus::on;
}

bool Interrupt::is_disabled()
{
    return get_status() == InterruptStatus::off;
}

void Interrupt::init()
{
    printkln("interrupt init start");
    init_idt_descript();  // 初始化中断描述符表
    init_idt_table();     // 异常名初始化并注册通常的中断处理函数
    init_pic();           // 初始化8259A

    /* 加载idt */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
    asm volatile("lidt %0" : : "m"(idt_operand));
    printkln("interrupt done");
}

AtomicGuard::AtomicGuard()
{
    old_status = Interrupt::get_status();
    Interrupt::disable();
}

AtomicGuard::~AtomicGuard()
{
    Interrupt::set_status(old_status);
}