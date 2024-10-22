#pragma once
#include "kernel/bitmap.h"
#include "kernel/list.h"

class Partition
{
public:
    void init(uint32_t start_lba, uint32_t sector_count, struct Disk* my_disk, const char* name);
    void format();
    bool is_valid();
    bool is_formated();
    void print_super_block_info();

public:
    const char* get_name();
    int32_t     alloc_block();
    void        free_block(uint32_t no);
    int32_t     alloc_inode();
    void        free_inode(uint32_t no);
    void        read_block_sector(uint32_t no, void* buffer);
    void        write_block_sector(uint32_t no, void* buffer);
    void        read_block_byte(uint32_t byte_offset, void* buffer, uint32_t count);
    void        write_block_byte(uint32_t byte_offset, void* buffer, uint32_t count);
    void        read_inode_sector(uint32_t no, void* buffer);
    void        write_inode_sector(uint32_t no, void* buffer);
    void        read_inode_byte(uint32_t byte_offset, void* buffer, uint32_t count);
    void        write_inode_byte(uint32_t byte_offset, void* buffer, uint32_t count);

private:
    void read_sector(uint32_t lba, void* buffer, uint32_t sector_count);
    void write_sector(uint32_t lba, void* buffer, uint32_t sector_count);
    void read_byte(uint32_t byte_address, void* buffer, uint32_t byte_count);
    void write_byte(uint32_t byte_address, void* buffer, uint32_t byte_count);

private:
    uint32_t     partition_lba_base;         // 起始扇区
    uint32_t     partition_sector_count;     // 扇区数
    uint32_t     block_start_sector;         // block起始扇区
    uint32_t     block_sector_count;         // block扇区数量
    uint32_t     inode_start_sector;         // inode起始扇区
    uint32_t     inode_sector_count;         // inode扇区数量
    uint32_t     inode_bitmap_start_sector;  // inode bitmap 起始扇区
    uint32_t     block_bitmap_start_sector;  // block bitmap 起始扇区
    struct Disk* my_disk;                    // 分区所属的硬盘
    char         name[8];                    // 分区名称
    Bitmap       block_bitmap;               // 块位图
    Bitmap       inode_bitmap;               // i结点位图
    bool         formated;                   // 是否格式化
    bool         valid;                      // 是否有效
};