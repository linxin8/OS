#pragma once

#include "lib/stdint.h"

uint32_t printf(const char* str, ...);
uint32_t vsprintf(char* str, const char* format, void* ap);
uint32_t sprintf(char* buf, const char* format, ...);
uint32_t printk(const char* format, ...);
uint32_t printk(uint32_t value);
uint32_t printk(int32_t value);
uint32_t printk(uint8_t ch);
uint32_t printk(int8_t ch);
uint32_t printk(void* pointer);
uint32_t printkln(uint32_t value);
uint32_t printkln(int32_t value);
uint32_t printkln(uint8_t ch);
uint32_t printkln(int8_t ch);
uint32_t printkln(const char* format, ...);
uint32_t printkln(void* pointer);
uint32_t printk_debug(const char* format, ...);