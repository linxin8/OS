#include "lib/stdlib.h"
#include "lib/syscall.h"

void* malloc(uint32_t size)
{
    return Systemcall::malloc(size);
}
void free(void* p)
{
    return Systemcall::free(p);
}
uint32_t getpid()
{
    return Systemcall::getpid();
}

void yeild()
{
    Systemcall::yield();
}

void sleep(uint32_t m_interval)
{
    Systemcall::sleep(m_interval);
}