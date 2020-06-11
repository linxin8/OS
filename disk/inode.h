#pragma once

#include "disk/ide.h"
#include "kernel/list.h"
#include "lib/stdint.h"

/* inode结构 */
struct Inode
{
    uint32_t no;  // inode编号
    /* 当此inode是文件时,i_size是指文件大小,
    若此inode是目录,i_size是指该目录下所有目录项大小之和*/
    uint32_t size;
    uint32_t open_count;  // 记录此文件被打开的次数
    bool     write_deny;  // 写文件不能并行,进程写文件前检查此标识
    /* i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针 */
    //总共12+512/4=140个块
    uint32_t    sector[13];
    ListElement list_tag;
};