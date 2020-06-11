#include "process/tss.h"
#include "kernel/asm_interface.h"
#include "kernel/boot_config.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/string.h"
#include "process/tss.h"
#include "thread/thread.h"

/* 任务状态段tss结构 */
struct
{
    uint32_t  backlink;
    uint32_t* esp0;
    uint32_t  ss0;
    uint32_t* esp1;
    uint32_t  ss1;
    uint32_t* esp2;
    uint32_t  ss2;
    uint32_t  cr3;
    uint32_t (*eip)();
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
} tss;

struct GdtDescript
{
    uint16_t limit_low_word;
    uint16_t base_low_word;
    uint8_t  base_mid_byte;
    uint8_t  attr_low_byte;
    uint8_t  limit_high_attr_high;
    uint8_t  base_high_byte;
};

/* 更新 tss 中 esp0 字段的值为 thread 的 0 级线 */
void TSS::update_esp0(PCB* pcb)
{
    tss.esp0 = (uint32_t*)((uint32_t)pcb + PAGE_SIZE);
}

GdtDescript make_gdt_descript(uint32_t* desc_addr, uint32_t limit, uint8_t attribute_low, uint8_t attribute_high)
{
    uint32_t    desc_base = (uint32_t)desc_addr;
    GdtDescript desc;
    desc.limit_low_word       = limit & 0x0000ffff;
    desc.base_low_word        = desc_base & 0x0000ffff;
    desc.base_mid_byte        = ((desc_base & 0x00ff0000) >> 16);
    desc.attr_low_byte        = (uint8_t)(attribute_low);
    desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attribute_high));
    desc.base_high_byte       = desc_base >> 24;
    return desc;
}

void TSS::init()
{
    printkln("tss init start");
    uint32_t tss_size = sizeof(tss);
    memset(&tss, 0, sizeof(tss));
    tss.ss0     = SELECTOR_K_STACK;
    tss.io_base = tss_size;
    /* gdt段基址为0x900,把tss放到第4个位置,也就是0x900+0x20的位置 */

    /* 在gdt中添加dpl为0的TSS描述符 */
    *((GdtDescript*)0xc0000920) = make_gdt_descript((uint32_t*)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

    /* 在gdt中添加dpl为3的数据段和代码段描述符 */
    *((GdtDescript*)0xc0000928) = make_gdt_descript((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    *((GdtDescript*)0xc0000930) = make_gdt_descript((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

    /* gdt 16位的limit 32位的段基址 */
    uint64_t gdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16));  // 7个描述符大小
    asm volatile("lgdt %0" : : "m"(gdt_operand));
    asm volatile("ltr %w0" : : "r"(SELECTOR_TSS));
    printkln("tss init and ltr done");
}