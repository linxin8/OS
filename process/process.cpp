#include "process/process.h"
#include "kernel/boot_config.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "lib/math.h"
#include "lib/stdio.h"
#include "process/tss.h"
#include "thread/sync.h"
#include "thread/thread.h"

#define THREAD_DEFAULT_PRIORITY 31
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
#define PG_P_1 1   // 页表项或页目录项存在属性位
#define PG_P_0 0   // 页表项或页目录项存在属性位
#define PG_RW_R 0  // R/W 属性位值, 读/执行
#define PG_RW_W 2  // R/W 属性位值, 读/写/执行
#define PG_US_S 0  // U/S 属性位值, 系统级
#define PG_US_U 4  // U/S 属性位值, 用户级

extern "C" {
void intr_exit();
};
/* 构建用户进程初始上下文信息 */
void process_entry(void* filename)
{
    void* function = filename;
    PCB*  thread   = Thread::get_current_pcb();
    thread->self_kstack += sizeof(ThreadStack);  //跳过thread stack
    InterruptStack* stack = (InterruptStack*)thread->self_kstack;
    stack->edi            = 0;
    stack->esi            = 0;
    stack->ebp            = 0;
    stack->esp_dummy      = 0;
    stack->ebx            = 0;
    stack->edx            = 0;
    stack->ecx            = 0;
    stack->eax            = 0;
    stack->gs             = 0;
    stack->ds             = SELECTOR_U_DATA;
    stack->es             = SELECTOR_U_DATA;
    stack->fs             = SELECTOR_U_DATA;
    stack->eip            = (decltype(stack->eip))function;
    stack->cs             = SELECTOR_U_CODE;
    // stack->cs = SELECTOR_K_CODE;  //为了调试方便，让用户程序能运行内核代码
    // printk_debug("debug setting: user can run kernel code\n");
    stack->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    //为用户内核栈分配内存
    Memory::malloc_physical_page_for_virtual_page(false, (void*)USER_STACK3_VADDR);
    stack->esp = (void*)(USER_STACK3_VADDR + PAGE_SIZE);
    stack->ss  = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(stack) : "memory");
}

/* 激活页表 */
void Process::activate_page_directory(PCB* thread)
{
    /********************************************************
     * 执行此函数时,当前任务可能是线程。
     * 之所以对线程也要重新安装页表, 原因是上一次被调度的可能是进程,
     * 否则不恢复页表的话,线程就会使用进程的页表了。
     ********************************************************/

    /* 若为内核线程,需要重新填充页表为0x100000 */
    uint32_t pgd_phy_addr = 0x100000;  // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
    if (thread->pgd != nullptr)
    {  // 用户态进程有自己的页目录表
        pgd_phy_addr = (uint32_t)Memory::get_phsical_address_by_virtual_address(thread->pgd);
    }

    /* 更新页目录寄存器cr3,使新页表生效 */
    asm volatile("movl %0, %%cr3" : : "r"(pgd_phy_addr) : "memory");
}

/* 激活线程或进程的页表,更新tss中的esp0为进程的特权级0的栈 */
void Process::activate(PCB* thread)
{
    ASSERT(thread != nullptr);
    /* 激活该进程或线程的页表 */
    activate_page_directory(thread);

    /* 内核线程特权级本身就是0,处理器进入中断时并不会从tss中获取0特权级栈地址,故不需要更新esp0 */
    if (thread->pgd)
    {
        /* 更新该进程的esp0,用于此进程被中断时保留上下文 */
        TSS::update_esp0(thread);
    }
}

/* 创建页目录表,复制内核的pde,
 * 成功则返回页目录的虚拟地址,否则返回nullptr */
uint32_t* create_page_dir()
{
    /* 用户进程的页表不能让用户直接访问到,所以在内核空间来申请 */
    uint32_t* page_dir_vaddr = (uint32_t*)Memory::malloc_kernel_page(1);
    if (page_dir_vaddr == nullptr)
    {
        ASSERT(false);  // create page dir: get_kernel_page failed!
        return nullptr;
    }
    /************************** 1  先复制页表  *************************************/
    /*  page_dir_vaddr + 0x300*4 是内核页目录的第768项,768项是内核第一个页项目，最后一个页项目是1023*/
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);
    /*****************************************************************************/

    /************************** 2  更新页目录地址 **********************************/
    uint32_t new_page_dir_phy_addr = (uint32_t)Memory::get_phsical_address_by_virtual_address(page_dir_vaddr);
    /* 页目录地址是存入在页目录的最后一项,更新页目录地址为新页目录的物理地址 */
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    /*****************************************************************************/
    return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(PCB* user_prog)
{
    user_prog->user_virutal_address_pool.start_address = (void*)USER_VADDR_START;
    uint32_t bitmap_pg_cnt = div_round_up((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    user_prog->user_virutal_address_pool.bitmap.start_address = (uint8_t*)Memory::malloc_kernel_page(bitmap_pg_cnt);
    user_prog->user_virutal_address_pool.bitmap.byte_size     = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    memset(user_prog->user_virutal_address_pool.bitmap.start_address, 0,
           user_prog->user_virutal_address_pool.bitmap.byte_size);
}

/* 创建用户进程 */
void Process::execute(void* filename, const char* process_name)
{
    AtomicGuard guard;
    PCB*        pcb = Thread::create_thread(process_name, THREAD_DEFAULT_PRIORITY, process_entry, filename);
    pcb->pgd        = create_page_dir();
    create_user_vaddr_bitmap(pcb);
    Memory::init_block_descript(pcb->user_block_descript);
}

pid_t Process::fork()
{
    AtomicGuard guard;
    ASSERT(Thread::is_current_user_thread());
    auto parent = Thread::get_current_pcb();
    auto child  = (PCB*)Memory::malloc_kernel_page(1);
    ASSERT(((uint32_t)child & 0XFFF) == 0);
    // auto child = (PCB*)Memory::malloc_kernel(PAGE_SIZE);
    ASSERT(child != nullptr);
    // auto buffer = Memory::malloc_kernel_page(1);
    auto buffer = (PCB*)Memory::malloc_kernel(PAGE_SIZE);
    ASSERT(buffer != nullptr);

    //处理pcb
    memcpy(child, parent, PAGE_SIZE);
    child->pid        = Thread::alloc_pid();
    child->status     = TaskStatus::ready;
    child->parent_pid = parent->pid;
    child->semaphore_tag.init();
    child->thread_list_tag.init();
    Memory::init_block_descript(child->user_block_descript);
    create_user_vaddr_bitmap(child);
    ASSERT(child->user_virutal_address_pool.start_address != nullptr);
    memcpy(child->user_virutal_address_pool.bitmap.start_address,
           parent->user_virutal_address_pool.bitmap.start_address, child->user_virutal_address_pool.bitmap.byte_size);

    //处理页表
    child->pgd = create_page_dir();
    ASSERT(child->pgd != nullptr);
    for (uint32_t i = 0; i < child->user_virutal_address_pool.bitmap.byte_size * 8; i++)
    {
        if (parent->user_virutal_address_pool.bitmap.test(i))
        {  //此虚页存在，把父进程的页拷贝过来
            uint32_t vaddr = i * PAGE_SIZE + (uint32_t)parent->user_virutal_address_pool.start_address;
            memcpy(buffer, (void*)vaddr, PAGE_SIZE);
            activate_page_directory(child);  //使用子进程的页表
            Memory::malloc_physical_page_for_virtual_page(
                false, (void*)vaddr);  //由于vaddr在内核，所以子进程也能用相同的虚地址访问
            activate_page_directory(child);  //分配后，会自动重新加载parent的页表，所以需要再次请求使用子进程的页表
            memcpy((void*)vaddr, buffer, PAGE_SIZE);
            activate_page_directory(parent);  //使用父进程的页表
        }
    }

    //处理返回值
    InterruptStack* stack = (InterruptStack*)((uint32_t)child + PAGE_SIZE - sizeof(InterruptStack));
    stack->eax            = 0;  // eax是返回值，子进程返回0

    *((uint32_t*)stack - 1) = (uint32_t)intr_exit;
    child->self_kstack      = (uint32_t*)stack - 5;  //中断返回时的栈顶

    // to do
    // update_inode_open_cnts
    // Debug::break_point();
    Memory::free_kernel(buffer);
    Thread::insert_ready_thread(child);
    return child->pid;
}