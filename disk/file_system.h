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

/* 标准输入输出描述符 */
enum class STDFD : uint32_t
{
    stdin,   // 0 标准输入
    stdout,  // 1 标准输出
    stderr   // 2 标准错误
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
    int32_t          read(int32_t file_id, void* buffer, uint32_t count);
    int32_t          write(int32_t file_id, const void* buffer, uint32_t count);
    int32_t          pipe(int32_t fd[2]);
}  // namespace FileSystem
