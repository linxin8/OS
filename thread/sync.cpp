#include "thread/sync.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "thread/thread.h"

Semaphore::Semaphore(uint8_t value)
{
    this->value = value;
}
void Semaphore::down()
{
    AtomicGuard guard;
    while (value == 0)
    {
        ASSERT(!waiters.find(Thread::get_current_pcb()->semaphore_tag));
        waiters.push_back(Thread::get_current_pcb()->semaphore_tag);
        Thread::block_current_thread();
    }
    value--;
    ASSERT(value == 0);
}
void Semaphore::up()
{
    AtomicGuard guard;
    ASSERT(value == 0);
    if (!waiters.is_empty())
    {
        PCB* thread_blocked = Thread::get_pcb_by_semaphore_tag(waiters.pop_front());
        Thread::unblock_thread(thread_blocked);
    }
    value++;
    ASSERT(value == 1);
}

bool Semaphore::is_downed()
{
    AtomicGuard guard;
    return value == 0;
}

Lock::Lock() : holder(nullptr), semaphore(1) {}

void Lock::lock()
{
    ASSERT(holder != Thread::get_current_pcb());
    semaphore.down();
    ASSERT(holder == nullptr);
    holder = Thread::get_current_pcb();
}
void Lock::unlock()
{
    ASSERT(holder == Thread::get_current_pcb());
    holder = nullptr;
    semaphore.up();
}

bool Lock::is_locked()
{
    return holder != nullptr;
}

LockGuard::LockGuard(Lock& lock) : plock(&lock)
{
    plock->lock();
}

LockGuard::~LockGuard()
{
    plock->unlock();
}