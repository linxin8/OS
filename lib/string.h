#pragma once

#include "lib/stdint.h"

void     memset(void* dest, uint8_t value, uint32_t size);
void     memcpy(void* dest, const void* src, uint32_t size);
char*    strcpy(char* dest, const char* src);
uint32_t strlen(const char* str);
int      strcmp(const char* str1, const char* str2);
char*    strchr(const char* str, int ch);
char*    strrchr(const char* str, int ch);
char*    strcat(char* dest, const char* src);
int      str_count(const char* str, int ch);

char* itoa(int32_t value, char* str, int32_t base);
char* uitoa(uint32_t value, char* str, int32_t base);