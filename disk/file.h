#pragma once
#include "lib/stdint.h"

/* 文件结构 */
class File
{
public:
    File(class Partition* partition, int32_t inode_no, uint8_t flag);
    File();
    ~File();
    void     write(const void* data, uint32_t count);
    void     read(void* data, uint32_t count);
    uint32_t get_size();
    uint32_t get_position();

private:
    class Inode* inode;
    uint32_t     position;  // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
    uint32_t     flag;
};
