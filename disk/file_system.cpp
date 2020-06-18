#include "disk/file_system.h"
#include "disk/directory.h"
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

#define DEBUG_ALOWAYS_FORMAT true

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
                    if (!p.is_formated() || DEBUG_ALOWAYS_FORMAT)
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
                    if (!p.is_formated() || DEBUG_ALOWAYS_FORMAT)
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

DirectoryEntry* find_entry(Directory& directory, const char* name) {}

Directory FileSystem::open_directory(const char* path)
{
    ASSERT(path[0] == '/');                //确保是绝对路径
    Directory parent(current_partion, 0);  //默认是当前分区
    char      buffer[20];
    char      len = 0;
    for (int i = 1; path[i] != '\0'; i++)
    {
        if (path[i] == '\\')
        {
            ASSERT(len != 0);
            auto entry = parent.read_entry(buffer);
            if (!entry.is_valid())
            {
                printk("%s not found\n", buffer);
                return Directory();
            }
            if (!entry.is_directory())
            {
                printk("%s is not directory\n", buffer);
                return Directory();
            }
            parent = Directory(parent.get_partition(), entry.get_inode_no());
        }
        else
        {
            buffer[len++] = path[i];
        }
    }
    return parent;
}

void FileSystem::debug_test()
{
    auto     root  = new Directory(current_partion, 0);
    uint32_t count = root->get_entry_count();
    PRINT_VAR(count)
    for (uint32_t i = 0; i < root->get_entry_count(); i++)
    {
        auto entry = root->read_entry(i);
        PRINT_VAR(entry.get_name());
        PRINT_VAR(entry.get_inode_no());
        PRINT_VAR(entry.get_type());
    }
    // char str[] = "my file     ";
    // for (int i = 0; i < 100; i++)
    // {
    //     itoa(i, str + 7, 10);
    //     auto temp = new DirectoryEntry(str, -1, DirectoryEntry::file);
    //     root->insert_entry(temp);
    // }
    // PRINT_VAR(root->get_entry_count());
    // for (uint32_t i = 0; i < root->get_entry_count(); i++)
    // {
    //     root->read_entry(i, entry);
    //     PRINT_VAR(entry->get_name());
    //     PRINT_VAR(entry->get_inode_no());
    //     PRINT_VAR(entry->get_type());
    // }
    printk('\n');
    while (true) {}
}