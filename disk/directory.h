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
    DirectoryEntry(const char* name, int32_t inode_no, Type type);
    const char* get_name() const;
    int32_t     get_inode_no() const;
    Type        get_type() const;

private:
    char    name[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
    int32_t inode_no;                 // 普通文件或目录对应的inode编号
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
    Directory(const Directory& directory);
    static Directory open_root_directory(class Partition* partition);
    ~Directory();
    void           close();
    bool           is_empty();
    uint32_t       get_entry_count() const;
    bool           is_valid() const;
    Directory      open_directory(const char* name);
    void           insert_directory(const char* name);
    void           insert_file(const char* name);
    DirectoryEntry read_entry(uint32_t index);
    void           remove_file(const char* name);
    void           remove_directory(const char* name);

private:
    static const uint32_t index_max;

private:
    Directory(class Partition* partition, int32_t inode_no);
    DirectoryEntry find_entry(const char* name);
    Directory(Inode* inode);
    void             insert_entry(const DirectoryEntry& entry);
    void             write_entry(uint32_t index, const DirectoryEntry& directory_entry);
    class Partition* get_partition() const;

private:
    Inode* inode;
};
