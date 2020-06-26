#include "disk/file_system.h"
#include "disk/directory.h"
#include "disk/file.h"
#include "disk/inode.h"
#include "kernel/keyboard.h"
#include "kernel/memory.h"
#include "kernel/pipe.h"
#include "lib/debug.h"
#include "lib/math.h"
#include "lib/string.h"
#include "thread/thread.h"

#define SUPER_BLOCK_MAGIC 0x12345678
#define MAX_FILES_PER_PART 4096  // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096     // 每扇区的位数
#define SECTOR_SIZE 512          // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE   // 块字节大小

#define DEBUG_ALOWAYS_FORMAT false

#define MAX_FILES_OPEN 32  //系统最大文件打开数量

enum class GlobalFileType : uint32_t
{
    file,
    pipe
};

struct GlobalFileDescript
{
    enum Type
    {
        file,
        pipe
    } type;
    void* data;
} global_file_descript[MAX_FILES_OPEN];

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

int32_t insert_thread_fd(int fd)
{
    auto pcb = Thread::get_current_pcb();
    for (int32_t i = 3; i < MAX_FILES_OPEN_PER_THREAD; i++)
    {
        if (pcb->file_table[i] == -1)
        {
            pcb->file_table[i] = fd;
            return i;
        }
    }
    return -1;
}

int32_t FileSystem::pipe(int32_t fd[2])
{
    int32_t index = -1;
    for (int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if (global_file_descript[i].data == nullptr)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
    {
        return -1;
    }
    global_file_descript[index].type = GlobalFileDescript::pipe;
    global_file_descript[index].data = new Pipe();
    fd[0]                            = insert_thread_fd(index);
    fd[1]                            = insert_thread_fd(index);
    ASSERT(fd[0] != -1 && fd[1] != -1);
    return 0;
}

int32_t FileSystem::read(int32_t fd, void* buffer, uint32_t count)
{
    ASSERT(buffer != nullptr);
    ASSERT(0 <= fd && fd < MAX_FILES_OPEN_PER_THREAD);
    ASSERT(fd != (int32_t)STDFD::stdout);
    ASSERT(fd != (int32_t)STDFD::stderr);
    auto global_file_id = Thread::get_current_pcb()->file_table[fd];
    ASSERT(0 <= global_file_id && global_file_id < MAX_FILES_OPEN);
    auto& descript = global_file_descript[global_file_id];
    if (descript.type == GlobalFileDescript::pipe)
    {
        ASSERT(descript.data != nullptr);
        auto pipe = (Pipe*)descript.data;
        pipe->read(buffer, count);
        return count;
    }
    else if (fd == (int32_t)STDFD::stdin)
    {
        char* data = (char*)buffer;
        for (uint32_t i = 0; i < count; i++)
        {
            data[i] = Keyboard::read_key(true);
        }
        return count;
    }
    else
    {
        ASSERT(descript.data != nullptr);
        auto file         = (File*)descript.data;
        auto byte_to_read = min(count, file->get_size() - file->get_position());
        if (byte_to_read == 0)
        {
            return -1;
        }
        file->read(buffer, byte_to_read);
        return byte_to_read;
    }
}

int32_t FileSystem::write(int32_t fd, const void* buffer, uint32_t count)
{
    ASSERT(buffer != nullptr);
    ASSERT(0 <= fd && fd < MAX_FILES_OPEN_PER_THREAD);
    auto global_file_id = Thread::get_current_pcb()->file_table[fd];
    ASSERT(0 <= global_file_id && global_file_id < MAX_FILES_OPEN);
    auto& descript = global_file_descript[global_file_id];
    if (descript.type == GlobalFileDescript::pipe)
    {
        ASSERT(descript.data != nullptr);
        auto pipe = (Pipe*)descript.data;
        pipe->write(buffer, count);
        return count;
    }
    else if (fd == (int32_t)STDFD::stdout || fd == (int32_t)STDFD::stderr)
    {
        char* data = (char*)buffer;
        for (uint32_t i = 0; i < count; i++)
        {
            printk("%c", data[i]);
        }
        return count;
    }
    else
    {
        ASSERT(descript.data != nullptr);
        auto file         = (File*)descript.data;
        auto byte_to_read = min(count, file->get_size() - file->get_position());
        if (byte_to_read == 0)
        {
            return -1;
        }
        file->write(buffer, byte_to_read);
        return byte_to_read;
    }
}
void FileSystem::init()
{
    for (int i = 0; i < MAX_FILES_OPEN; i++)
    {
        global_file_descript[i].data = nullptr;
    }
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

void ls(Directory directory, uint32_t level = 0)
{
    ASSERT(directory.is_valid());
    // PRINT_VAR(directory.get_entry_count());
    for (uint32_t i = 0; i < directory.get_entry_count(); i++)
    {
        auto entry = directory.read_entry(i);
        ASSERT(entry.is_valid());
        if (entry.is_directory())
        {
            for (uint32_t i = 0; i < level; i++)
            {
                printk("  ");
            }
            printk("d %s %d\n", entry.get_name(), entry.get_inode_no());
            if (i >= 2)
            {  //第0个和第1个目录是. 和 ..
                ls(directory.open_directory(entry.get_name()), level + 1);
            }
        }
        else if (entry.is_file())
        {
            for (uint32_t i = 0; i < level; i++)
            {
                printk("  ");
            }
            printk("f %s %d\n", entry.get_name(), entry.get_inode_no());
        }
    }
}

void FileSystem::debug_test()
{
    auto root = Directory::open_root_directory(current_partion);
    ls(root);
    // PRINT_VAR(root.get_entry_count());
    // Debug::break_point();
    // Debug::break_point();
    // ls(root);
    // printkln("====================");
    // root.insert_directory("233");
    // root.insert_file("root file");
    // auto sub = root.open_directory("233");
    // sub.insert_directory("abc");
    // sub.insert_file("sub file");
    // auto sub2 = sub.open_directory("abc");
    // sub2.insert_file("sub2 file");
    // sub2.insert_file("aaa");
    // ls(root);
    // root.insert_file("file1");
    // root.insert_directory("dir1");
    // auto dir = root.open_directory("dir1");
    // dir.insert_directory("dir2");
    // auto dir2 = dir.open_directory("dir2");
    // dir2.insert_file("file2");
    printkln("\nok");
    while (true) {}
}