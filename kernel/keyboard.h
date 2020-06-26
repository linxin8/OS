#pragma once
#include "lib/stdint.h"

namespace Keyboard
{
    void init();
    char read_key(bool block = false);
}  // namespace Keyboard
