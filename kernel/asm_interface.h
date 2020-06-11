#pragma once

#include "lib/stdio.h"
extern "C" {
void    user_main(void* arg);
void    put_char(uint8_t character);
void    put_str(const char* str);
void    put_int(uint32_t number);
void    set_cursor(uint32_t cursor_pos);
void    cls_screen(void);
void    outb(uint16_t port, uint8_t data);
void    outsw(uint16_t port, const void* addr, uint32_t word_cnt);
uint8_t inb(uint16_t port);
void    insw(uint16_t port, void* addr, uint32_t word_cnt);
void    switch_to(struct PCB* current_thread, struct PCB* next_thread);
}