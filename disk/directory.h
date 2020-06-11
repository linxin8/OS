#pragma once

#include "disk/file_system.h"
#include "disk/ide.h"
#include "disk/inode.h"
#include "lib/stdint.h"

#define MAX_FILE_NAME_LEN 16  // 最大文件名长度

/* 目录结构 */
struct Directory
{
    Inode*   inode;
    uint32_t offset;                 // 记录在目录内的偏移
    uint8_t  directory_buffer[512];  // 目录的数据缓存
};

/* 目录项结构 */
struct DirectoryEntry
{
    char     file_name[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
    uint32_t inode_number;                  // 普通文件或目录对应的inode编号
    FileType file_type;                     // 文件类型
};

extern Directory root_directory;  // 根目录
void             open_root_directory(Partition* part);
Directory*       directory_open(Partition* part, uint32_t inode_number);
void             directory_close(Directory* directory);
bool search_directory_entry(Partition* part, Directory* directory, const char* name, DirectoryEntry* directory_entry);
void create_directory_entry(char* filename, uint32_t inode_no, FileType file_type, DirectoryEntry* directory_entry);
bool sync_directory_entry(Directory* parent_directory, DirectoryEntry* directory_entry, void* io_buffer);
bool delete_directory_entry(Partition* part, Directory* directory, uint32_t inode_no, void* io_buf);
DirectoryEntry* directory_read(Directory* directory);
bool            directory_is_empty(Directory* directory);
int32_t         directory_remove(Directory* parent_directory, Directory* child_directory);