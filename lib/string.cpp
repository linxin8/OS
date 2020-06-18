#include "lib/string.h"
#include "lib/debug.h"
#include "lib/stdio.h"

#define NULL '\0'

void memset(void* dest, uint8_t value, uint32_t size)
{
    ASSERT(dest != nullptr);
    uint8_t* p = (uint8_t*)dest;
    while (size--)
    {
        *p++ = value;
    }
}

void memcpy(void* dst, const void* src, uint32_t size)
{
    ASSERT(dst != nullptr);
    ASSERT(src != nullptr);
    const uint8_t* psrc = (const uint8_t*)src;
    uint8_t*       pdst = (uint8_t*)dst;
    while (size--)
    {
        *(uint8_t*)pdst++ = *(uint8_t*)psrc++;
    }
}

int memcmp(const void* str1, const void* str2, uint32_t size)
{
    ASSERT(str1 != nullptr);
    ASSERT(str2 != nullptr);
    uint8_t* p1 = (uint8_t*)str1;
    uint8_t* p2 = (uint8_t*)str2;
    while (size--)
    {
        if (*p1 > *p2)
        {
            return 1;
        }
        else if (*p1 < *p2)
        {
            return -1;
        }
        p1++;
        p2++;
    }
    return 0;
}

char* strcpy(char* dest, const char* src)
{
    ASSERT(dest != nullptr);
    ASSERT(src != nullptr);
    char* ret = dest;
    while (*src != NULL)
    {
        *dest++ = *src++;
    }
    *dest = NULL;
    return ret;
}

uint32_t strlen(const char* str)
{
    ASSERT(str != nullptr);
    uint32_t ret = 0;
    while (*str != NULL)
    {
        str++;
        ret++;
    }
    return ret;
}

int strcmp(const char* str1, const char* str2)
{
    ASSERT(str1 != nullptr);
    ASSERT(str2 != nullptr);
    while (*str1 != NULL && *str1 == *str2)
    {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

char* strchr(const char* str, int ch)
{
    ASSERT(str != nullptr);
    ch = (char)ch;
    while (*str != ch && *str != NULL)
    {
        str++;
    }
    if (*str == NULL)
    {
        return nullptr;
    }
    return (char*)str;
}

char* strrchr(const char* str, int ch)
{
    ASSERT(str != nullptr);
    ch        = (char)ch;
    char* ret = nullptr;
    while (*str != NULL)
    {
        if (*str == ch)
        {
            ret = (char*)str;
        }
        str++;
    }
    return ret;
}

char* strcat(char* dest, const char* src)
{
    ASSERT(dest != nullptr);
    ASSERT(src != nullptr);
    while (*dest != NULL)
    {
        dest++;
    }
    while (*src != NULL)
    {
        *dest++ = *src++;
    }
    *dest = NULL;
    return dest;
}

int str_count(const char* str, int ch)
{
    ASSERT(str != nullptr);
    ch      = (char)ch;
    int ret = 0;
    while (*str != NULL)
    {
        if (*str == ch)
        {
            ret++;
        }
    }
    return ret;
}

/* 将有符号整型转换成字符串*/
char* itoa(int32_t value, char* str, int32_t base)
{
    char  index[] = "0123456789ABCDEF";
    char* ret     = str;
    int   pos     = 0;
    if (base == 10 && value < 0) /*十进制负数*/
    {
        value      = -value;
        str[pos++] = '-';
    }
    do
    {
        str[pos++] = index[value % base];
        value /= base;
    } while (value != 0);
    str[pos] = '\0';
    /*逆序*/
    int start = 0;
    if (ret[0] == '-')
    {
        start = 1; /*十进制负数*/
    }
    int temp = 0;
    for (int end = pos - 1; start < end; start++, end--)
    {
        temp       = str[start];
        str[start] = str[end];
        str[end]   = temp;
    }
    return ret;
}

/* 将无符号整型转换成字符串*/
char* uitoa(uint32_t value, char* str, int32_t base)
{
    char  index[] = "0123456789ABCDEF";
    char* ret     = str;
    int   pos     = 0;
    do
    {
        str[pos++] = index[value % base];
        value /= base;
    } while (value != 0);
    str[pos] = '\0';
    /*逆序*/
    int start = 0;
    int temp  = 0;
    for (int end = pos - 1; start < end; start++, end--)
    {
        temp       = str[start];
        str[start] = str[end];
        str[end]   = temp;
    }
    return ret;
}