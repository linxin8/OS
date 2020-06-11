#pragma once
#include "lib/stdint.h"

namespace Keyboard
{
    void init();
    int8_t read_key(bool block = false);
}  // namespace Keyboard
