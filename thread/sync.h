#pragma once

#include "kernel/list.h"
#include "lib/stdint.h"

struct PCB;

class Semaphore
{
public:
    Semaphore(uint8_t value = 1);
    void up();
    void down();
    bool is_downed();

private:
    uint32_t value;
    List     waiters;
};

class Lock
{
public:
    Lock();
    void lock();
    void unlock();
    bool is_locked();

private:
    PCB*      holder;
    Semaphore semaphore;
};

class LockGuard
{
public:
    LockGuard(Lock& lock);
    ~LockGuard();

private:
    Lock* plock;
};

#ifndef CriticalSectionGuard
#    define __critical_make_lock_variable(line) static Lock __critical_lock##line
#    define __critical_make_lock_guard_variable(line) LockGuard __critical_lock_guard##line(__critical_lock##line)
#    define __critical_section_guard(line)                                                                             \
        __critical_make_lock_variable(line);                                                                           \
        __critical_make_lock_guard_variable(line);
//临界区，只允许至多一个线程进入
#    define CriticalSectionGuard() __critical_section_guard(__LINE__);
#endif