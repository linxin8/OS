#pragma once
#include "lib/stdint.h"

void panic_spin(const char* filename, uint32_t line, const char* func, const char* condition);

/***************************  __VA_ARGS__  *******************************
 * __VA_ARGS__ 是预处理器所支持的专用标识符。
 * 代表所有与省略号相对应的参数.
 * "..."表示定义的宏其参数可变.*/
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)
/***********************************************************************/

#ifdef NDEBUG
#    define ASSERT(CONDITION) ((void)0)
#else
#    define ASSERT(CONDITION)                                                                                          \
        if (CONDITION) {}                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            /* 符号#让编译器将宏的参数转化为字符串字面量 */                                        \
            PANIC(#CONDITION);                                                                                         \
        }
#endif /*__NDEBUG */

#define ABORT() ASSERT(false)
namespace Debug
{
    void break_point(bool condition = true);
    void hlt();
    void enter_line(const char* filename, const char* func, uint32_t line);
}  // namespace Debug

#define PRINT_VAR(x)                                                                                                   \
    printk(#x ": ");                                                                                                   \
    printkln(x);

#define PRINT_VAR_2(a, b) PRINT_VAR(a) PRINT_VAR(b)

#define PRINT_VAR_3(a, b, c) PRINT_VAR(a) PRINT_VAR(b) PRINT_VAR(c)

#define LOG_LINE() Debug::enter_line(__FILE__, __func__, __LINE__);