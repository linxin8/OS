#include "kernel/init.h"
#include "console.h"
#include "disk/file_system.h"
#include "disk/ide.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "kernel/keyboard.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "lib/stdio.h"
#include "lib/syscall.h"
#include "process/tss.h"
#include "thread/thread.h"

void init_all()
{
    printkln("init all start");
    //初始化中断
    Interrupt::init();
    Timer::init();
    Memory::init();
    Thread::init();
    TSS::init();
    Systemcall::init();
    Keyboard::init();
    Interrupt::enable();
    IDE::init();
    FileSystem::init();
    // Interrupt::disable();
    // idt_init();
    // timer_init();
    // memory_init();
    // thread_init();
    // console_init();
    // keyboard_init();
    // tss_init();
    // syscall_init();
    // interrupt_enable();  // 后面的ide_init需要打开中断
    // ide_init();
    // file_system_init();
    printkln("init all done");
}