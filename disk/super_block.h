#pragma once

#include "lib/stdint.h"

// 超级块,位于分区第一个扇区
struct SuperBlock
{
    uint32_t magic;  // 用来标识文件系统类型,支持多文件系统的操作系统通过此标志来识别文件系统类型
    uint32_t sector_count;        // 本分区总共的扇区数
    uint32_t inode_count;         // 本分区中inode数量
    uint32_t partition_lba_base;  // 本分区的起始lba地址

    uint32_t block_bitmap_lba;     // 块位图本身起始扇区地址
    uint32_t block_bitmap_sector;  // 扇区位图本身占用的扇区数量

    uint32_t inode_bitmap_lba;     // i结点位图起始扇区lba地址
    uint32_t inode_bitmap_sector;  // i结点位图占用的扇区数量

    uint32_t inode_table_lba;     // i结点表起始扇区lba地址
    uint32_t inode_table_sector;  // i结点表占用的扇区数量

    uint32_t block_start_lba;    // 数据区开始的第一个扇区号
    uint32_t root_inode_number;  // 根目录所在的I结点号
    uint32_t dir_entry_size;     // 目录项大小

    uint8_t pad[460];  // 加上460字节,凑够512字节1扇区大小
} __attribute__((packed));
// static_assert(sizeof(SuperBlock) == 512);
