#include "disk/inode.h"
#include "disk/partition.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/math.h"

#define SECTOR_SIZE 512  // 扇区字节大小

Inode::Inode(Partition* partition, uint32_t no)
{
    partition->read_inode_byte(no * sizeof(Inode), this, sizeof(Inode));
    this->no              = no;
    this->partition       = partition;
    this->reference_count = 1;
    this->list_tag.init();
    this->is_write_deny = false;
    this->extend_sector = nullptr;
    if (sector[13] != -1)
    {
        this->extend_sector = (int32_t*)Memory::malloc(SECTOR_SIZE);
        partition->read_inode_sector(sector[13], this->extend_sector);
    }
}

uint32_t Inode::get_no() const
{
    return no;
}

void Inode::write(uint32_t byte_index, const void* src, uint32_t count)
{
    ASSERT(src != nullptr);
    ASSERT(byte_index <= size);
    uint32_t first_sector      = byte_index / SECTOR_SIZE;
    uint32_t first_byte_offset = byte_index % SECTOR_SIZE;
    uint32_t last_sector       = (byte_index + count - 1) / SECTOR_SIZE;
    uint32_t last_byte_offset  = (byte_index + count - 1) % SECTOR_SIZE;
    ASSERT(last_sector < 140);
    for (uint32_t i = 0; i < first_sector; i++)
    {  //确保之前的商区不为空
        ASSERT(get_block_index(i) != -1);
    }
    if (first_byte_offset != 0)
    {
        ASSERT(get_block_index(first_sector) != -1);
    }
    for (uint32_t i = first_sector; i <= last_sector; i++)
    {  //分配扇区
        int32_t& index = get_block_index(i);
        if (index == -1)
        {
            index = partition->alloc_block();
            if (i == 12)
            {
                ASSERT(extend_sector == nullptr);
                extend_sector = (int32_t*)Memory::malloc(SECTOR_SIZE);
                for (uint32_t i = 0; i < SECTOR_SIZE / sizeof(int8_t); i++)
                {
                    extend_sector[i] = -1;
                }
            }
        }
    }
    uint8_t* p_data = (uint8_t*)src;
    for (uint32_t i = first_sector; i <= last_sector; i++)
    {
        int32_t index = get_block_index(i);
        if (i == first_sector)
        {
            uint32_t len = SECTOR_SIZE - first_byte_offset;
            if (first_sector == last_sector)
            {
                ASSERT(last_byte_offset >= first_byte_offset);
                len = last_byte_offset - first_byte_offset + 1;
            }
            partition->write_block_byte(index * SECTOR_SIZE + first_byte_offset, p_data, len);
            p_data += len;
        }
        else if (i == last_sector)
        {
            partition->write_block_byte(index * SECTOR_SIZE, p_data, last_byte_offset + 1);
            p_data += last_byte_offset + 1;
        }
        else
        {
            partition->write_block_byte(index * SECTOR_SIZE, p_data, SECTOR_SIZE);
            p_data += SECTOR_SIZE;
        }
    }
    ASSERT((uint32_t)p_data - (uint32_t)src == count);
    size = max(size, byte_index + count);
}
void Inode::read(uint32_t byte_index, void* des, uint32_t count)
{
    ASSERT(des != nullptr);
    ASSERT(byte_index + count - 1 < size);
    uint32_t first_sector      = byte_index / SECTOR_SIZE;
    uint32_t first_byte_offset = byte_index % SECTOR_SIZE;
    uint32_t last_sector       = (byte_index + count - 1) / SECTOR_SIZE;
    uint32_t last_byte_offset  = (byte_index + count - 1) % SECTOR_SIZE;
    ASSERT(last_sector < 140);
    for (uint32_t i = 0; i <= last_sector; i++)
    {
        ASSERT(get_block_index(i) != -1);  //确保扇区不为空
    }
    uint8_t* p_data = (uint8_t*)des;
    for (uint32_t i = first_sector; i <= last_sector; i++)
    {
        int32_t index = get_block_index(i);
        if (i == first_sector)
        {
            uint32_t len = SECTOR_SIZE - first_byte_offset;
            if (first_sector == last_sector)
            {
                ASSERT(last_byte_offset >= first_byte_offset);
                len = last_byte_offset - first_byte_offset + 1;
            }
            partition->read_block_byte(index * SECTOR_SIZE + first_byte_offset, p_data, len);
            p_data += len;
        }
        else if (i == last_sector)
        {
            partition->read_block_byte(index * SECTOR_SIZE, p_data, last_byte_offset + 1);
            p_data += last_byte_offset + 1;
        }
        else
        {
            partition->read_block_byte(index * SECTOR_SIZE, p_data, SECTOR_SIZE);
            p_data += SECTOR_SIZE;
        }
    }
    ASSERT((uint32_t)p_data - (uint32_t)des == count);
}

int32_t& Inode::get_block_index(uint32_t index)
{
    ASSERT(index < 140);
    if (index < 12)
    {
        return sector[index];
    }
    return extend_sector[index - 12];
}

Inode::~Inode()
{
    reference_count--;
    if (reference_count == 0)
    {
        if (extend_sector != nullptr)
        {
            Memory::free(extend_sector);
        }
    }
}

uint32_t Inode::get_size() const
{
    return size;
}

Partition* Inode::get_partition()
{
    return partition;
}