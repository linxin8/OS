#pragma once
#include "disk/directory.h"
#include "disk/file.h"
#include "ide.h"
#include "lib/stdint.h"

/* 打开文件的选项 */
enum class FileOpenFlag : uint32_t
{
    read_only,   // 只读
    write_only,  // 只写
    read_write,  // 读写
    create = 4   // 创建
};

/* 文件读写位置偏移量 */
enum class FileWhence : uint32_t
{
    start = 1,
    current,
    end
};

/* 用来记录查找文件过程中已找到的上级路径,也就是查找文件过程中"走过的地方" */
struct PathSearchRecord
{
    char             searched_path[512];  // 查找过程中的父路径
    class Directory* parent_directory;    // 文件或目录所在的直接父目录
    // FileType         file_type;  // 找到的是普通文件还是目录,找不到将为未知类型(FT_UNKNOWN)
};

/* 文件属性结构体 */
struct FileState
{
    uint32_t inode_number;  // inode编号
    uint32_t size;          // 尺寸
    // FileType type;          // 文件类型
};

namespace FileSystem
{
    void             init();
    void             mount_partition(const char* name);
    void             debug_test();
    int32_t          open(const char* path, uint8_t flag);
    class Partition* get_current_partition();
    Directory        open_directory(const char* path);
    Directory        insert_directory(const char* parent_path, const char* directory_name);
    File             insert_file(const char* parent_path, const char* file_name);
}  // namespace FileSystem
