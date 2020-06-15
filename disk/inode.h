#pragma once

#include "disk/ide.h"
#include "kernel/list.h"
#include "lib/stdint.h"

/* inode结构 */
class Inode
{
public:
    Inode(uint32_t no, class Partition* partition);
    ~Inode();
    void write(uint32_t byte_index, void* src, uint32_t count);
    void read(uint32_t byte_index, void* des, uint32_t count);

private:
    int32_t& get_block_index(uint32_t index);

private:
    uint32_t no;  // inode编号
    /* 当此inode是文件时,i_size是指文件大小,
    若此inode是目录,i_size是指该目录下所有目录项大小之和*/
    uint32_t size;
    uint32_t reference_count;  //  引用计数器,计数为0时，释放内存
    bool     is_write_deny;    // 写文件不能并行,进程写文件前检查此标识
    /* i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针 */
    //总共12+512/4=140个块
    int32_t          sector[13];
    ListElement      list_tag;
    int32_t*         extend_sector;
    class Partition* partition;
};