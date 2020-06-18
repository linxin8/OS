#pragma once

#include "disk/file_system.h"
#include "disk/ide.h"
#include "disk/inode.h"
#include "lib/stdint.h"

#define MAX_FILE_NAME_LEN 16  // 最大文件名长度

/* 目录项结构 */
struct DirectoryEntry
{
    friend class Partition;

public:
    enum Type : uint32_t
    {
        none,      // 不存在
        file,      // 普通文件
        directory  // 目录
    };

public:
    DirectoryEntry();
    DirectoryEntry(const char* name, uint32_t inode_no, Type type);
    const char* get_name() const;
    uint32_t    get_inode_no() const;
    Type        get_type() const;

private:
    char     name[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
    uint32_t inode_no;                 // 普通文件或目录对应的inode编号
    /* 文件类型 */
    Type type;  // 文件类型
public:
    bool is_valid() const;
    bool is_directory() const;
    bool is_file() const;
};

/* 目录结构 */
class Directory
{
public:
    Directory();
    Directory(class Partition* partition, uint32_t inode_no);
    Directory(Directory&& direction);
    ~Directory();
    void             close();
    DirectoryEntry   read_entry(const char* name);
    bool             sync_entry(Directory* parent_directory, DirectoryEntry* directory_entry, void* io_buffer);
    bool             delete_entry(uint32_t inode_no, void* io_buf);
    void             insert_entry(DirectoryEntry* directory_entry);
    bool             is_empty();
    uint32_t         get_entry_count() const;
    DirectoryEntry   read_entry(uint32_t index);
    void             write_entry(uint32_t index, const DirectoryEntry& directory_entry);
    class Partition* get_partition() const;
    bool             is_valid() const;
    Directory&       operator=(Directory&& directory);

private:
    static const uint32_t index_max;

private:
    Directory(Inode* inode);

private:
    Inode* inode;
    // uint32_t offset;  // 记录在目录内的偏移
    // uint8_t  directory_buffer[512];  // 目录的数据缓存
};
