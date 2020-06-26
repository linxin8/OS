#pragma once
#include "lib/stdio.h"

void*   malloc(uint32_t size);
void    free(void* p);
int16_t getpid();
void    yeild();
void    sleep(uint32_t m_interval);
int16_t fork();
int32_t pipe(int32_t fd[2]);
int32_t read(int32_t fd, void* buffer, uint32_t count);
int32_t write(int32_t fd, const void* buffer, uint32_t count);