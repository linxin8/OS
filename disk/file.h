#pragma once
#include "lib/stdint.h"

#define MAX_FILE_OPEN 32  // 系统可打开的最大文件数

/* 标准输入输出描述符 */
enum class STDFD : uint32_t
{
    stdin,   // 0 标准输入
    stdout,  // 1 标准输出
    stderr   // 2 标准错误
};

/* 文件结构 */
class File
{
public:
    File(class Partition* partition, uint32_t inode_no, uint8_t flag);
    ~File();
    void     write(const void* data, uint32_t count);
    void     read(void* data, uint32_t count);
    uint32_t get_size();

private:
    uint32_t     position;  // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
    uint32_t     flag;
    class Inode* inode;
};
