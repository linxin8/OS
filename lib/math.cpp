#include "lib/math.h"

uint32_t div_round_up(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}