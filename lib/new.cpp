#include "kernel/interrupt.h"
#include "kernel/memory.h"
#include "lib/stdlib.h"

void* operator new(__SIZE_TYPE__ size)
{
    if (Interrupt::is_enabled())
    {
        return malloc(size);
    }
    return Memory::malloc(size);
}

void operator delete(void* pointer)
{
    if (Interrupt::is_enabled())
    {
        return free(pointer);
    }
    return Memory::free(pointer);
}

void operator delete(void* pointer, unsigned int)
{
    if (Interrupt::is_enabled())
    {
        return free(pointer);
    }
    return Memory::free(pointer);
}