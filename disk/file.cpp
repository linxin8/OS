#include "disk/file.h"
#include "disk/file_system.h"
#include "disk/inode.h"
#include "disk/partition.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/stdlib.h"
#include "lib/string.h"

#define BLOCK_SIZE 512

void File::close()
{
    inode->write_deny = false;
    FileSystem::get_current_partition()->close_inode(inode);
    inode = nullptr;
}

uint32_t& get_block_partition_index(uint32_t no, uint32_t* block, uint32_t* extend_block)
{
    ASSERT(block != nullptr);
    ASSERT(no < 140);
    if (no < 12)
    {
        return block[no];
    }
    ASSERT(extend_block != nullptr);
    return extend_block[no - 12];
}

int32_t File::write(const void* data, uint32_t count)
{
    ASSERT(inode != nullptr);
    uint32_t first_block       = position / BLOCK_SIZE;
    uint32_t first_offset      = position % BLOCK_SIZE;
    uint32_t last_block        = (position + count - 1) / BLOCK_SIZE;
    uint32_t last_block_offset = (position + count - 1) % BLOCK_SIZE;
    auto     partition         = FileSystem::get_current_partition();
    ASSERT(last_block < 140);  //文件大小 小于140个块

    uint32_t* extend_block = nullptr;
    if (last_block >= 12)
    {  //读取拓展块
        ASSERT(inode->sector[12] != 0);
        extend_block = (uint32_t*)Memory::malloc(BLOCK_SIZE);
        partition->read_block(inode->sector[12], extend_block);
    }

    for (uint32_t i = 0; i < first_block; i++)
    {  //之前的快应该是存在的
        uint32_t index = get_block_partition_index(i, inode->sector, extend_block);
        ASSERT(index != 0);
    }

    if (first_offset != 0)
    {  //当前块也是应该存在的
        uint32_t index = get_block_partition_index(first_block, inode->sector, extend_block);
        ASSERT(index != 0);
    }

    //开始分配块
    for (uint32_t i = first_block; i <= last_block; i++)
    {
        uint32_t index = get_block_partition_index(first_block, inode->sector, extend_block);
        if (index != 0)
        {  //如果带写入的块不存在，则分配一个块
            index = partition->alloc_block();
        }
    }

    //开始写入块
    auto     block_buffer = Memory::malloc(BLOCK_SIZE);
    uint8_t* p_data       = (uint8_t*)data;

    for (uint32_t i = first_block; i <= last_block; i++)
    {
        uint32_t index = get_block_partition_index(i, inode->sector, extend_block);
        if (i == first_block)
        {  //写入第一个块
            partition->read_block(index, block_buffer);
            memcpy(block_buffer, p_data, BLOCK_SIZE - first_offset);
            partition->write_block(index, block_buffer);
            p_data += BLOCK_SIZE - first_offset;
        }
        else if (i == last_block)
        {  //写入最后一个块
            partition->read_block(index, block_buffer);
            memcpy(block_buffer, p_data, last_block_offset + 1);
            partition->write_block(index, block_buffer);
            p_data += last_block_offset + 1;
        }
        else
        {  //写入中间块
            partition->write_block(index, p_data);
            p_data += BLOCK_SIZE;
        }
    }
    ASSERT((uint32_t)(p_data) - (uint32_t)(data) == count);
    if (extend_block != nullptr)
    {
        Memory::free(extend_block);
    }
    Memory::free(block_buffer);
    return count;
}

int32_t File::read(void* data, uint32_t count)
{
    ASSERT(inode != nullptr);
    uint32_t first_block       = position / BLOCK_SIZE;
    uint32_t first_offset      = position % BLOCK_SIZE;
    uint32_t last_block        = (position + count - 1) / BLOCK_SIZE;
    uint32_t last_block_offset = (position + count - 1) % BLOCK_SIZE;
    auto     partition         = FileSystem::get_current_partition();
    ASSERT(last_block < 140);  //文件大小 小于140个块

    uint32_t* extend_block = nullptr;
    if (last_block >= 12)
    {  //读取拓展块
        ASSERT(inode->sector[12] != 0);
        extend_block = (uint32_t*)Memory::malloc(BLOCK_SIZE);
        partition->read_block(inode->sector[12], extend_block);
    }

    for (uint32_t i = 0; i <= last_block; i++)
    {  //到last block之前的快应该是存在的
        uint32_t index = get_block_partition_index(i, inode->sector, extend_block);
        ASSERT(index != 0);
    }

    //开始读取块
    uint8_t* block_buffer = (uint8_t*)Memory::malloc(BLOCK_SIZE);
    uint8_t* p_data       = (uint8_t*)data;

    for (uint32_t i = first_block; i <= last_block; i++)
    {
        uint32_t index = get_block_partition_index(i, inode->sector, extend_block);
        if (i == first_block)
        {  //读取第一个块
            partition->read_block(index, block_buffer);
            memcpy(p_data, block_buffer + first_offset, BLOCK_SIZE - first_offset);
            p_data += BLOCK_SIZE - first_offset;
        }
        else if (i == last_block)
        {  //读取最后一个块
            partition->read_block(index, block_buffer);
            memcpy(p_data, block_buffer, last_block_offset + 1);
            p_data += last_block_offset + 1;
        }
        else
        {  //读取中间块
            partition->read_block(index, p_data);
            p_data += BLOCK_SIZE;
        }
    }
    ASSERT((uint32_t)(p_data) - (uint32_t)(data) == count);
    if (extend_block != nullptr)
    {
        Memory::free(extend_block);
    }
    Memory::free(block_buffer);
    return count;
}