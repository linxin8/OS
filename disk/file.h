#pragma once
#include "lib/stdint.h"

#define MAX_FILE_OPEN 32  // 系统可打开的最大文件数

/* 文件结构 */
class File
{
public:
    // int32_t file_create(struct Directory* parent_dir, const char* filename, uint8_t flag);
    //  void    bitmap_sync(class Partition* partition, uint32_t bit_idx, BitmapType bitmap_type);
    int32_t get_free_slot_in_global(void);
    int32_t pcb_fd_install(int32_t globa_fd_idx);
    int32_t open(uint32_t inode_no, uint8_t flag);
    void    close();
    int32_t write(const void* data, uint32_t count);
    int32_t read(void* data, uint32_t count);

private:
    uint32_t     position;  // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
    uint32_t     flag;
    class Inode* inode;
};

/* 标准输入输出描述符 */
enum class STDFD : uint32_t
{
    stdin,   // 0 标准输入
    stdout,  // 1 标准输出
    stderr   // 2 标准错误
};