#include "kernel/pipe.h"
#include "kernel/interrupt.h"
#include "kernel/io_queue.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdlib.h"

Pipe::Pipe()
{
    AtomicGuard guard;
    queue  = (IOQueue*)Memory::malloc_kernel(sizeof(queue));  //把队列放在内核中，方便所有进程共享
    *queue = IOQueue();                                       //初始化
    reference_count = 1;
}

Pipe::~Pipe()
{
    --reference_count;
    if (reference_count == 0)
    {
        Memory::free_kernel(queue);  //从内核中释放内存
        queue = nullptr;
    }
}

void Pipe::add_reference_count()
{
    AtomicGuard guard;
    reference_count++;
}

void Pipe::read(void* buffer, uint32_t count)
{
    char* data = (char*)buffer;
    for (uint32_t i = 0; i < count; i++)
    {
        data[i] = queue->pop_front();
    }
}
void Pipe::write(const void* buffer, uint32_t count)
{
    const char* data = (const char*)buffer;
    for (uint32_t i = 0; i < count; i++)
    {
        queue->push_back(data[i]);
    }
}

//为了共享pipe，需要把inode放在内核空间中
void* Pipe::operator new(__SIZE_TYPE__ size)
{
    return Memory::malloc_kernel(size);
}

//与 new 对应，需要在内核空间中释放内存
void Pipe::operator delete(void* p)
{
    Memory::free_kernel(p);
}