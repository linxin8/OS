#include "kernel/log.h"
#include "lib/stdio.h"

// #define LOG_ENABLE

#ifndef LOG_ENABLE
#    define LOG_CHECK() return;
#else
#    define LOG_CHECK()
#endif

void Log::thread_switch(const char* current_name, const char* next_name)
{
    LOG_CHECK();
    // bool enable = false;
    bool enable = true;
    if (enable)
    {
        printk("thread switch %s -> %s\n", current_name, next_name);
    }
}