#pragma once
#include "lib/stdio.h"

void*    malloc(uint32_t size);
void     free(void* p);
uint32_t getpid();
void     yeild();
void     sleep(uint32_t m_interval);