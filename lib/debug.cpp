#include "lib/debug.h"
#include "kernel/interrupt.h"
#include "lib/stdio.h"

// condition为true时，进入断点
void Debug::break_point(bool condition)
{
    if (condition)
    {
        asm volatile("xchg %%bx,%%bx" ::);
    }
}

//暂停程序,释放bochs的cpu占用
void Debug::hlt()
{
    // bochs不支持hlt
    // asm volatile("hlt" ::);
    while (true)
    {  //通过断点来释放bochs软件对cpu的占用
        asm volatile("xchg %%bx,%%bx" ::);
    }
}

/* 打印文件名,行号,函数名,条件并使程序悬停 */
void panic_spin(const char* filename, uint32_t line, const char* func, const char* condition)
{
    Interrupt::disable();  // 因为有时候会单独调用panic_spin,所以在此处关中断。
    printk("\nASSERT %s %s %d ", filename, func, line);
    printk("failed: %s\n", condition);
    Debug::hlt();
}

void Debug::enter_line(const char* filename, const char* func, uint32_t line)
{
    printkln("log %s %s %d ", filename, func, line);
}