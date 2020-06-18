#include "disk/file.h"
#include "disk/file_system.h"
#include "disk/inode.h"
#include "disk/partition.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/stdlib.h"
#include "lib/string.h"

File::File(Partition* partition, uint32_t inode_no, uint8_t flag)
{
    inode    = new Inode(partition, inode_no);
    position = 0;
}

File::~File()
{
    delete inode;
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

void File::write(const void* data, uint32_t count)
{
    inode->write(position, data, count);
}

void File::read(void* data, uint32_t count)
{
    inode->read(position, data, count);
}

uint32_t File::get_size()
{
    return inode->get_size();
}