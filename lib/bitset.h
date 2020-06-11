#pragma once

#include "lib/stdint.h"

// #define BITMAP_MASK 1

// typedef struct Bitset
// {
//     size_t   length;
//     uint8_t* data;
// } Bitset;

// void bitset_init(Bitset* bitset);
// bool bitset_test(Bitset* bitset, size_t pos);

class Bitset
{
public:
    Bitset(uint32_t bit_size);

public:
    uint8_t*       data;
    const uint32_t bit_size;
};
