#include "lib/stdio.h"
#include "kernel/console.h"
#include "lib/debug.h"
#include "lib/string.h"
#include "lib/syscall.h"

// 把varg指向第一个固定参数v
#define va_start(varg, v) varg = (void*)&v
// varg指向下一个参数并返回其值
#define va_next_arg(varg, t) *((t*)(varg += 4))
// 清除varg
#define va_end(varg) varg = nullptr

uint32_t vsprintf(char* str, const char* format, void* _varg)
{
    char              current_char = *format;
    const char* const pstr         = str;
    char*             varg         = (char*)_varg;
    while (current_char != '\0')
    {
        if (current_char != '%')  //直接输出
        {
            *str++ = current_char;
        }
        else
        {
            current_char = *(++format);  // 得到%后面的字符
            switch (current_char)
            {
                case 's':
                    strcpy(str, va_next_arg(varg, const char*));
                    while (*str != '\0')
                    {
                        str++;
                    };
                    break;
                case 'c': *(str++) = va_next_arg(varg, char); break;
                case 'd':
                    itoa(va_next_arg(varg, int32_t), str, 10);
                    while (*str != '\0')
                    {
                        str++;
                    };
                    break;
                case 'x':
                case 'p':
                    uitoa(va_next_arg(varg, uint32_t), str, 16);
                    while (*str != '\0')
                    {
                        str++;
                    };
                    break;
            }
        }
        current_char = *(++format);
    }
    *str = '\0';
    return str - pstr;
}

uint32_t sprintf(char* buff, const char* format, ...)
{
    return vsprintf(buff, format, &format);
}

/* 格式化输出字符串format */
uint32_t printf(const char* format, ...)
{
    char     buff[1024];  // 用于存储拼接后的字符串
    uint32_t len = vsprintf(buff, format, &format);
    ASSERT(len < 1024 - 1);
    // return write(1, buf, strlen(buf));
    return Systemcall::write(buff);
}

/* 格式化输出字符串format */
uint32_t printk(const char* format, ...)
{
    char     buff[128];  // 用于存储拼接后的字符串
    uint32_t len = vsprintf(buff, format, &format);
    ASSERT(len < 128 - 1);
    Console::put_str(buff);
    return len;
}

uint32_t printk_debug(const char* format, ...)
{
    char     buff[128];  // 用于存储拼接后的字符串
    uint32_t len = vsprintf(buff, format, &format);
    ASSERT(len < 128 - 1);
    Console::put_str(buff);
    return len;
}

uint32_t printkln(uint32_t value)
{
    return printk("%x\n", value);
}

uint32_t printkln(const char* format, ...)
{
    char     buff[128];  // 用于存储拼接后的字符串
    uint32_t len = vsprintf(buff, format, &format);
    buff[len++]  = '\n';
    buff[len++]  = '\0';
    ASSERT(len < 128);
    Console::put_str(buff);
    return len;
}

uint32_t printkln(int32_t value)
{
    return printk("%d\n", value);
}

uint32_t printkln(void* p)
{
    return printk("%x\n", p);
}

uint32_t printkln(uint8_t ch)
{
    return printk("%c\n", ch);
}

uint32_t printkln(int8_t ch)
{
    return printk("%c\n", ch);
}

uint32_t printk(uint32_t value)
{
    return printk("%x", value);
}

uint32_t printk(int32_t value)
{
    return printk("%d", value);
}

uint32_t printk(uint8_t ch)
{
    return printk("%c", ch);
}

uint32_t printk(int8_t ch)
{
    return printk("%c", ch);
}

uint32_t printk(void* p)
{
    return printk("%x", p);
}
