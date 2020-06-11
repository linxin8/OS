#pragma once

#include "lib/stdint.h"

uint32_t div_round_up(uint32_t a, uint32_t b);

template <typename T>
const T& max(const T& left, const T& right)
{
    return left > right ? left : right;
}

template <typename T>
const T& min(const T& left, const T& right)
{
    return left < right ? left : right;
}
