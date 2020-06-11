#include "lib/syscall.h"
#include "kernel/console.h"
// #include "kernel/print.h"
#include "kernel/asm_interface.h"
#include "kernel/timer.h"
#include "lib/debug.h"
#include "lib/stdint.h"
#include "lib/stdio.h"
#include "lib/string.h"

typedef void* Syscall_t;

enum class SystemcallType : uint32_t
{
    getpid,
    write,
    malloc,
    free,
    fork,
    read,
    putchar,
    clear,
    getcwd,
    open,
    close,
    lseek,
    unlink,
    mkdir,
    opendir,
    closedir,
    chdir,
    rmdir,
    readdir,
    rewinddir,
    stat,
    ps,
    execv,
    exit,
    wait,
    pipe,
    fd_redirect,
    help,
    yield,
    sleep,
    max,
};

Syscall_t syscall_table[(uint32_t)SystemcallType::max];

/* 无参数的系统调用 */
#define _syscall0(NUMBER)                                                                                              \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER) : "memory");                                             \
        retval;                                                                                                        \
    })

/* 一个参数的系统调用 */
#define _syscall1(NUMBER, ARG1)                                                                                        \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1) : "memory");                                  \
        retval;                                                                                                        \
    })

/* 两个参数的系统调用 */
#define _syscall2(NUMBER, ARG1, ARG2)                                                                                  \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2) : "memory");                       \
        retval;                                                                                                        \
    })

/* 返回当前任务pid */
pid_t Systemcall::getpid()
{
    return _syscall0(SystemcallType::getpid);
}

static pid_t getpid()
{
    return Thread::get_current_pcb()->pid;
}

uint32_t Systemcall::write(const char* str)
{
    return _syscall1(SystemcallType::write, str);
}

uint32_t write(const char* str)
{
    Console::put_str(str);
    return strlen(str);
}

void* Systemcall::malloc(uint32_t size)
{
    return (void*)_syscall1(SystemcallType::malloc, size);
}

void Systemcall::free(void* p)
{
    _syscall1(SystemcallType::free, p);
}

void Systemcall::yield()
{
    _syscall0(SystemcallType::yield);
}

void Systemcall::sleep(uint32_t m_interval)
{
    _syscall1(SystemcallType::sleep, m_interval);
}

void Systemcall::init()
{
    printkln("systcall_init start");
    syscall_table[(uint32_t)SystemcallType::getpid] = (Syscall_t) & ::getpid;
    syscall_table[(uint32_t)SystemcallType::write]  = (Syscall_t) & ::write;
    syscall_table[(uint32_t)SystemcallType::malloc] = (Syscall_t)&Memory::malloc;
    syscall_table[(uint32_t)SystemcallType::free]   = (Syscall_t)&Memory::free;
    syscall_table[(uint32_t)SystemcallType::yield]  = (Syscall_t)&Thread::yield;
    syscall_table[(uint32_t)SystemcallType::sleep]  = (Syscall_t)&Timer::sleep;

    printkln("systcall_init done");
}