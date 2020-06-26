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
int16_t getpid()
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

int16_t fork()
{
    return Systemcall::fork();
}

int32_t pipe(int32_t fd[2])
{
    return Systemcall::pipe(fd);
}

int32_t read(int32_t fd, void* buffer, uint32_t count)
{
    return Systemcall::read(fd, buffer, count);
}

int32_t write(int32_t fd, const void* buffer, uint32_t count)
{
    return Systemcall::write(fd, buffer, count);
}