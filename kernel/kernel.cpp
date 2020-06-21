#include "disk/file_system.h"
#include "kernel/asm_interface.h"
#include "kernel/init.h"
#include "kernel/interrupt.h"
#include "kernel/keyboard.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "lib/debug.h"
#include "lib/macro.h"
#include "lib/new.h"
#include "lib/stdint.h"
#include "lib/stdio.h"
#include "lib/stdlib.h"
#include "process/process.h"
#include "thread/sync.h"
#include "thread/thread.h"

extern "C" {
void _init();
}

int i = 0;

int main()
{
    init_all();
    printf("main pid %d\n", getpid());
    // printf("main pid %x\n", &Thread::get_current_pcb()->pid);
    // printf("main pid %d\n", Thread::get_current_pcb()->pid);
    // printf("main pid %d\n", getpid());
    Process::execute((void*)user_main, "u1");
    // Process::execute((void*)user_main, "u2");
    // FileSystem::debug_test();
    // while (true) {}
    // Thread::create_thread("u1", 32, user_main, nullptr);
    Interrupt::enable();
    // Thread::create_thread("u2", 32, user_main, nullptr);
    while (true)
    {
        Thread::yield();
    }
    return 0;
}

void debug_interrupt_handle(uint32_t no)
{
    UNUSED(no);
}

void user_main(void* arg)
{
    UNUSED(arg);
    printf("\nuser process pid %d\n\n", getpid());
    int32_t test = 0;
    while (true)
    {
        auto pid = fork();
        if (pid == 0)
        {
            test--;
            printf("i am child %d, test addr: %x, value %d\n\n", getpid(), &test, test);
            while (true) {}
        }
        test++;
        printf("i am parent %d, child %d, test addr: %x, value %d\n\n", getpid(), pid, &test, test);
        while (true) {}
    }
}
