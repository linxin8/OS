#include "kernel/memory.h"
#include "lib/stdlib.h"

void* operator new(__SIZE_TYPE__ size)
{
    return malloc(size);
}

void operator delete(void* pointer)
{
    free(pointer);
}

void operator delete(void* pointer, unsigned int)
{
    free(pointer);
}