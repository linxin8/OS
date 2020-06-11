#include "kernel/console.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "thread/sync.h"

void Console::put_str(const char* str)
{
    AtomicGuard gurad; 
    ::put_str(str);
}
void Console::put_char(uint8_t character)
{
    AtomicGuard gurad;
    ::put_char(character);
} 