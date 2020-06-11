#include "disk/file_system.h"
#include "disk/file.h"
#include "disk/inode.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/math.h"
#include "lib/string.h"

#define SUPER_BLOCK_MAGIC 0x12345678
#define MAX_FILES_PER_PART 4096  // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096     // 每扇区的位数
#define SECTOR_SIZE 512          // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE   // 块字节大小

#define MAX_PATH_LEN 512  // 路径最大长度

Partition* sdb1;
Partition* sdb2;
Partition* current_partion;

Partition* find_partition(const char* name)
{
    for (int32_t ch = 0; ch < 2; ch++)
    {
        for (int32_t dev = 0; dev < 2; dev++)
        {
            if (dev == 0)
            {  //跳过操作系统硬盘
                continue;
            }
            Disk& disk = IDE::channel[ch].disk[dev];
            for (auto& p : disk.prim_parts)
            {
                if (p.is_valid())
                {
                    if (!strcmp(name, p.get_name()))
                    {
                        return &p;
                    }
                }
            }
            for (auto& p : disk.logic_parts)
            {
                if (p.is_valid())
                {
                    if (!strcmp(name, p.get_name()))
                    {
                        return &p;
                    }
                }
            }
        }
    }
    return nullptr;
}

void FileSystem::mount_partition(const char* name)
{
    Partition* p = find_partition(name);
    ASSERT(p != nullptr);
    current_partion = p;
}

int32_t FileSystem::open(const char* path, uint8_t flag)
{
    /* 对目录要用dir_open,这里只有open文件 */
    if (path[strlen(path) - 1] == '/')
    {
        printk("can`t open a directory %s\n", path);
        return -1;
    }
    ASSERT(flag <= 7);
    int32_t fd = -1;  // 默认为找不到
    return fd;        // to do
}

Partition* FileSystem::get_current_partition()
{
    return current_partion;
}

void FileSystem::init()
{
    for (int32_t ch = 0; ch < 2; ch++)
    {
        for (int32_t dev = 0; dev < 2; dev++)
        {
            if (dev == 0)
            {  //跳过操作系统硬盘
                continue;
            }
            Disk& disk = IDE::channel[ch].disk[dev];
            for (auto& p : disk.prim_parts)
            {
                if (p.is_valid())
                {
                    if (!p.is_formated())
                    {
                        p.format();
                    }
                    p.print_super_block_info();
                }
            }
            for (auto& p : disk.logic_parts)
            {
                if (p.is_valid())
                {
                    if (!p.is_formated())
                    {
                        p.format();
                    }
                    p.print_super_block_info();
                }
            }
        }
    }
    sdb1 = find_partition("sdb1");
    sdb2 = find_partition("sdb2");
    mount_partition("sdb1");
}

void FileSystem::debug_test()
{
    //
    // current_partion->print_super_block_info();
    auto inode = current_partion->open_inode(0);
    printkln("%x %x %x %x %x %x", inode, inode->no, inode->open_count, inode->sector, inode->size, inode->write_deny);
    current_partion->close_inode(inode);
    inode = current_partion->open_inode(0);
    printkln("%x %x %x %x %x %x", inode, inode->no, inode->open_count, inode->sector, inode->size, inode->write_deny);

    auto buffer = Memory::malloc(BLOCK_SIZE);
    // current_partion->read_block(0, buffer);
    // memcpy(buffer, "abcdef123456", 10);
    // current_partion->write_block(0, buffer);
    current_partion->read_block(0, buffer);
    for (int i = 0; i < 10; i++)
    {
        printk(((uint8_t*)buffer)[i]);
    }
    printk('\n');
    while (true) {}
}