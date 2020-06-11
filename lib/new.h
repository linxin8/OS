#pragma once
//__SIZE_TYPE__是long unsigned int, gcc编译器特性，
//不用__SIZE_TYPE__时会有警告，所以添加了
void* operator new(__SIZE_TYPE__ size);
void  operator delete(void* pointer);
void  operator delete(void* pointer, unsigned int);