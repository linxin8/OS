#pragma once

#include "disk/partition.h"
#include "kernel/bitmap.h"
#include "kernel/list.h"
#include "lib/stdio.h"
#include "thread/sync.h"
 
/* 硬盘结构 */
struct Disk
{
    char               name[8];         // 本硬盘的名称，如sda等
    struct IDEChannel* my_channel;      // 此块硬盘归属于哪个ide通道
    uint8_t            dev_no;          // 本硬盘是主0还是从1
    Partition          prim_parts[4];   // 主分区顶多是4个
    Partition          logic_parts[8];  // 逻辑分区数量无限,但总得有个支持的上限,那就支持8个
};

/* ata通道结构 */
struct IDEChannel
{
    char name[8];  // 本ata通道名称, 如ata0,也被叫做ide0. 可以参考bochs配置文件中关于硬盘的配置。
    uint16_t  port_base;  // 本通道的起始端口号
    uint8_t   irq_no;     // 本通道所用的中断号
    Lock      lock;
    bool      expecting_intr;  // 向硬盘发完命令后等待来自硬盘的中断
    Semaphore disk_done;  // 硬盘处理完成.线程用这个信号量来阻塞自己，由硬盘完成后产生的中断将线程唤醒
    Disk      disk[2];  // 一个通道上连接两个硬盘，一主一从
};

namespace IDE
{
    void              init(void);
    extern uint8_t    channel_count;
    extern IDEChannel channel[2];
    extern List       partition_list;
    void              read(Disk* hd, uint32_t lba, void* buf, uint32_t sector_count);
    void              write(Disk* hd, uint32_t lba, void* buf, uint32_t sector_count);
}  // namespace IDE
