#include "kernel/bitmap.h"
#include "lib/debug.h"
bool Bitmap::test(uint32_t bit_index)
{
    ASSERT(bit_index < byte_size * 8);
    uint32_t byte_index = bit_index / 8;  // 向下取整用于索引数组下标
    uint32_t bit_offset = bit_index % 8;  // 取余用于索引数组内的位
    return start_address[byte_index] & (1 << bit_offset);
}

// 搜索连续的count个位,返回起始位bit下标,失败时返回-1
int32_t Bitmap::scan(uint32_t count)
{
    uint32_t byte_index = 0;
    while ((byte_index < byte_size) && (0xff == start_address[byte_index]))
    { /* 1表示该位已分配,所以若为0xff,则表示该字节内已无空闲位,向下一字节继续找 */
        byte_index++;
    }
    if (byte_index == byte_size)
    {  // 找不到可用空间
        return -1;
    }
    uint32_t bit_offset = 0;
    while ((uint8_t)(1 << bit_offset) & start_address[byte_index])
    {  //跳过值为1的位
        bit_offset++;
    }
    uint32_t bit_index_start = byte_index * 8 + bit_offset;  // 空闲位在位图内的下标
    if (count == 1)
    {
        return bit_index_start;
    }
    uint32_t bit_left       = (byte_size * 8 - bit_index_start);  // 记录一共剩下多少位可以判断
    uint32_t next_bit_index = bit_index_start + 1;
    uint32_t free_count     = 1;   // 用于记录找到的空闲位的个数
    bit_index_start         = -1;  // 先将其置为-1,若找不到连续的位就直接返回
    while (bit_left-- > 0)
    {
        if (!(test(next_bit_index)))
        {  // 若next_bit为0
            free_count++;
        }
        else
        {
            free_count = 0;
        }
        if (free_count == count)
        {  // 若找到连续的count个空位
            bit_index_start = next_bit_index - count + 1;
            break;
        }
        next_bit_index++;
    }
    return bit_index_start;
}

//设置第i个bit的值
void Bitmap::set(uint32_t bit_index, bool value)
{
    ASSERT(bit_index < byte_size * 8);
    uint32_t byte_index = bit_index / 8;  // 向下取整用于索引数组下标
    uint32_t bit_offset = bit_index % 8;  // 取余用于索引数组内的位
    if (value)
    {
        start_address[byte_index] |= (1 << bit_offset);
    }
    else
    {
        start_address[byte_index] &= ~(1 << bit_offset);
    }
}

//填充多个bit的值
void Bitmap::fill(uint32_t bit_start_index, uint32_t count, bool value)
{
    for (uint32_t i = 0; i < count; i++)
    {
        set(bit_start_index + i, value);
    }
}

//手动初始化，size是位图的大小，address是位图的内存起始地址
void Bitmap::init(uint32_t size, uint8_t* address)
{
    byte_size     = size;
    start_address = address;
    memset(address, 0, byte_size);
}