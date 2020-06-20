#include "disk/file.h"
#include "disk/file_system.h"
#include "disk/inode.h"
#include "disk/partition.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/stdlib.h"
#include "lib/string.h"

File::File() : inode(nullptr), position(0), flag(0) {}

File::File(Partition* partition, int32_t inode_no, uint8_t flag)
    : inode(Inode::get_instance(partition, inode_no)), position(0), flag(flag)
{
}

File::~File()
{
    if (inode != nullptr)
    {
        Inode::remove_instance(inode);
        inode = nullptr;
    }
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