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

void memory_debug(uint32_t interval = 1000)
{
    UNUSED(interval);
    auto pid = getpid();
}

int main()
{
    init_all();
    // Process::execute((void*)user_main, "u1");
    // Process::execute((void*)user_main, "u2");
    FileSystem::debug_test();
    while (true) {}
    Thread::create_thread("u1", 32, user_main, nullptr);
    Thread::create_thread("u2", 32, user_main, nullptr);
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
    printf("user_main %d\n", getpid());
    while (true)
    {
        memory_debug();
        printf("%c", Keyboard::read_key(true));
    }
}
