#include "kernel/io_queue.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "thread/thread.h"

IOQueue::IOQueue() : head(0), tail(0) {}

uint32_t get_next_position(uint32_t pos)
{
    uint32_t next = pos + 1;
    if (next >= IO_QUEUE_BUFF_SIZE)
    {
        return 0;
    }
    return next;
}

bool IOQueue::is_full()
{
    return get_next_position(head) == tail;
}

bool IOQueue::is_empty()
{
    return head == tail;
}

uint8_t IOQueue::pop_front()
{
    AtomicGuard guard;
    while (is_empty())
    {
        Thread::yield();
    }
    uint8_t byte = buff[tail];
    tail         = get_next_position(tail);
    return byte;
}

void IOQueue::push_back(uint8_t byte)
{
    AtomicGuard guard;
    while (is_full())
    {
        Thread::yield();
    }
    buff[head] = byte;
    head       = get_next_position(head);
}

uint32_t IOQueue::get_length()
{
    if (head >= tail)
    {
        return head - tail;
    }
    return IO_QUEUE_BUFF_SIZE - (tail - head);
}