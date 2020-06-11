#pragma once

#include "lib/stdint.h"
#include "lib/string.h"

//位图，需要手动初始化以及管理内存
class Bitmap
{
public:
    bool    test(uint32_t bit_index);
    int32_t scan(uint32_t count);
    void    set(uint32_t bit_index, bool value);
    void    fill(uint32_t bit_start_index, uint32_t count, bool value);
    void    init(uint32_t size, uint8_t* address);

public:
    uint32_t byte_size     = 0;
    uint8_t* start_address = nullptr;
};