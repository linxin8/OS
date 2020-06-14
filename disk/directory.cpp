#include "disk/directory.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/string.h"

Directory root;

void Directory::open_root_directory()
{
    root.inode  = FileSystem::get_current_partition()->open_inode(0);
    root.offset = 0;
}

Directory* Directory::open(uint32_t inode_no)
{
    Directory* dir = (Directory*)Memory::malloc(sizeof(Directory));
    dir->inode     = inode;
    dir->offset    = 0;
    return dir;
}
void Directory::close()
{
    if (this != &root)
    {
        FileSystem::get_current_partition()->close_inode(root.inode);
    }
}