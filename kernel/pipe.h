#pragma once
#include "lib/stdint.h"

class Pipe
{
public:
    Pipe();
    ~Pipe();
    void  read(void* buffer, uint32_t count);
    void  write(const void* buffer, uint32_t count);
    void* operator new(__SIZE_TYPE__ size);
    void  operator delete(void* p);
    void  add_reference_count();

private:
    class IOQueue* queue;
    int32_t        reference_count;
};