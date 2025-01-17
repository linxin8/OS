#pragma once

#include "disk/ide.h"
#include "kernel/list.h"
#include "lib/stdint.h"

/* inode结构,使用new/delete管理内存 */
class Inode
{
    friend class Partition;

public:
    static Inode*    get_instance(class Partition* partition, int32_t no);
    static Inode*    copy_instance(Inode* inode);
    static void      remove_instance(Inode* inode);
    void             write(uint32_t byte_index, const void* src, uint32_t count);
    void             read(uint32_t byte_index, void* des, uint32_t count);
    uint32_t         get_size() const;
    class Partition* get_partition();
    int32_t          get_no() const;

private:
    int32_t&     get_block_index(uint32_t index);
    void         save();
    static List& get_list();
    void*        operator new(__SIZE_TYPE__ size);
    void         operator delete(void* p);

private:
    Inode(class Partition* partition, int32_t no);
    ~Inode();
    int32_t no;  // inode编号
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