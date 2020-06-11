#include "disk/partition.h"
#include "disk/directory.h"
#include "disk/ide.h"
#include "disk/inode.h"
#include "disk/super_block.h"
#include "kernel/interrupt.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/math.h"
#include "lib/stdio.h"
#include "lib/string.h"

#define SUPER_BLOCK_MAGIC 0x12345678
#define MAX_FILES_PER_PART 4096  // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096     // 每扇区的位数
#define SECTOR_SIZE 512          // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE   // 块字节大小

Partition::Partition()
{
    this->partition_lba_base     = 0;
    this->partition_sector_count = 0;
    this->my_disk                = nullptr;
    valid                        = false;
    formated                     = false;
}

Partition::Partition(uint32_t start_lba, uint32_t sector_count, struct Disk* my_disk, const char* name)
{
    this->partition_lba_base     = start_lba;
    this->partition_sector_count = sector_count;
    this->my_disk                = my_disk;
    valid                        = sector_count > 0;
    formated                     = false;
    ASSERT(strlen(name) < 7);
    strcpy(this->name, name);
    if (sector_count != 0)
    {
        SuperBlock* super_block = (SuperBlock*)Memory::malloc(sizeof(SuperBlock));
        read_sector(1, super_block, 1);
        if (super_block->magic == SUPER_BLOCK_MAGIC)
        {  // super block 合法，说明已经格式化了，简单检查格式化内容是否正确
            if (super_block->partition_lba_base == this->partition_lba_base &&
                super_block->sector_count == this->partition_sector_count)
            {
                formated                   = true;
                block_bitmap.start_address = (uint8_t*)Memory::malloc(super_block->block_bitmap_sector * SECTOR_SIZE);
                block_bitmap.byte_size     = super_block->block_bitmap_sector * SECTOR_SIZE;
                block_start_sector         = super_block->block_start_lba;
                block_sector_count         = super_block->sector_count - block_start_sector;
                inode_start_sector         = super_block->inode_table_lba;
                inode_sector_count         = super_block->inode_bitmap_sector;
                read_sector(super_block->block_bitmap_lba, block_bitmap.start_address,
                            super_block->block_bitmap_sector);
                LOG_LINE();
                inode_bitmap.start_address = (uint8_t*)Memory::malloc(super_block->inode_bitmap_sector * SECTOR_SIZE);
                inode_bitmap.byte_size     = super_block->inode_table_sector * SECTOR_SIZE;
                read_sector(super_block->inode_bitmap_lba, inode_bitmap.start_address,
                            super_block->inode_bitmap_sector);
            }
            else
            {
                printkln("super block info is not match bios info!");
                ASSERT(false);
            }
        }
        Memory::free(super_block);
    }
}

Partition& Partition::operator=(Partition&& right)
{
    this->partition_lba_base     = right.partition_lba_base;
    this->partition_sector_count = right.partition_sector_count;
    this->my_disk                = right.my_disk;
    strcpy(name, right.name);
    this->block_bitmap       = right.block_bitmap;
    this->inode_bitmap       = right.inode_bitmap;
    this->inode_list         = (List &&) right.inode_list;
    this->formated           = right.formated;
    this->valid              = right.valid;
    this->block_sector_count = right.block_sector_count;
    this->block_start_sector = right.block_start_sector;
    this->inode_start_sector = right.inode_start_sector;
    this->inode_sector_count = right.inode_sector_count;
    return *this;
}

const char* Partition::get_name()
{
    return name;
}

uint32_t Partition::alloc_block()
{
    int32_t index = block_bitmap.scan(1);
    ASSERT(index != -1);
    block_bitmap.set(index, true);

    uint32_t byte = block_start_sector * BLOCK_SIZE + index / 8;
    ASSERT(byte <= block_sector_count * SECTOR_SIZE);
    write_byte(byte, block_bitmap.start_address + index / 8, 1);
    return index;
}

//读取一个数据块
void Partition::read_block(uint32_t no, void* buffer)
{
    ASSERT(no < block_sector_count);
    ASSERT(block_bitmap.test(no));
    uint32_t block = block_start_sector + no;
    read_sector(block, buffer, 1);
}

//写入一个数据块
void Partition::write_block(uint32_t no, void* buffer)
{
    ASSERT(no < block_sector_count);
    ASSERT(block_bitmap.test(no));
    uint32_t block = block_start_sector + no;
    write_sector(block, buffer, 1);
}

void Partition::format()
{
    /* 为方便实现,一个块大小是一扇区 */
    uint32_t boot_sector_sector  = 1;
    uint32_t super_block_sector  = 1;
    uint32_t inode_bitmap_sector = div_round_up(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sector  = div_round_up(sizeof(Inode) * MAX_FILES_PER_PART, SECTOR_SIZE);
    uint32_t used_sector         = boot_sector_sector + super_block_sector + inode_bitmap_sector + inode_table_sector;
    uint32_t free_sector         = partition_sector_count - used_sector;

    /************** 简单处理块位图占据的扇区数 ***************/
    uint32_t block_bitmap_sector = div_round_up(free_sector, BITS_PER_SECTOR);
    /* block_bitmap_bit_len是位图中位的长度,也是可用块的数量 */
    uint32_t block_bitmap_bit_length = free_sector - block_bitmap_sector;
    block_bitmap_sector              = div_round_up(block_bitmap_bit_length, BITS_PER_SECTOR);

    /* 超级块初始化 */
    SuperBlock sb;
    sb.magic              = SUPER_BLOCK_MAGIC;
    sb.sector_count       = partition_sector_count;
    sb.inode_count        = MAX_FILES_PER_PART;
    sb.partition_lba_base = partition_lba_base;

    sb.block_bitmap_lba    = 2;  // 第0块是引导块,第1块是超级块
    sb.block_bitmap_sector = block_bitmap_sector;

    sb.inode_bitmap_lba    = sb.block_bitmap_lba + sb.block_bitmap_sector;
    sb.inode_bitmap_sector = inode_bitmap_sector;

    sb.inode_table_lba    = sb.inode_bitmap_lba + sb.inode_bitmap_sector;
    sb.inode_table_sector = inode_table_sector;

    sb.block_start_lba   = sb.inode_table_lba + sb.inode_table_sector;
    sb.root_inode_number = 0;  //根目录的 inode 编号为 0
    sb.dir_entry_size    = sizeof(Directory);

    /*******************************
     * 1 将超级块写入本分区的1扇区 *
     ******************************/
    write_sector(1, &sb, 1);
    /* 找出数据量最大的元信息,用其尺寸做存储缓冲区*/
    // buffer太大，栈装不下，用堆
    uint32_t buffer_size =
        (sb.block_bitmap_sector >= sb.inode_bitmap_sector ? sb.block_bitmap_sector : sb.inode_bitmap_sector);
    buffer_size     = (buffer_size >= sb.inode_table_sector ? buffer_size : sb.inode_table_sector) * SECTOR_SIZE;
    uint8_t* buffer = (uint8_t*)Memory::malloc(buffer_size);  // 申请的内存由内存管理系统清0后返回
    memset(buffer, 0, buffer_size);
    /**************************************
     * 2 将块位图初始化并写入sb.block_bitmap_lba *
     *************************************/
    /* 初始化块位图block_bitmap */
    buffer[0] |= 0x01;  // 第0个块预留给根目录,位图中先占位
    uint32_t block_bitmap_last_byte = block_bitmap_bit_length / 8;
    uint8_t  block_bitmap_last_bit  = block_bitmap_bit_length % 8;
    uint32_t last_size              = SECTOR_SIZE - (block_bitmap_last_byte %
                                        SECTOR_SIZE);  // last_size是位图所在最后一个扇区中，不足一扇区的其余部分

    /* 1 先将位图最后一字节到其所在的扇区的结束全置为1,即超出实际块数的部分直接置为已占用*/
    memset(&buffer[block_bitmap_last_byte], 0xff, last_size);

    /* 2 再将上一步中覆盖的最后一字节内的有效位重新置0 */
    uint8_t bit_idx = 0;
    while (bit_idx <= block_bitmap_last_bit)
    {
        buffer[block_bitmap_last_byte] &= ~(1 << bit_idx++);
    }
    write_sector(sb.block_bitmap_lba, buffer, sb.block_bitmap_sector);

    /***************************************
     * 3 将inode位图初始化并写入sb.inode_bitmap_lba *
     ***************************************/
    /* 先清空缓冲区*/
    memset(buffer, 0, buffer_size);
    buffer[0] |= 0x1;  // 第0个inode分给了根目录
    /* 由于inode_table中共4096个inode,位图inode_bitmap正好占用1扇区,
     * 即inode_bitmap_sects等于1, 所以位图中的位全都代表inode_table中的inode,
     * 无须再像block_bitmap那样单独处理最后一扇区的剩余部分,
     * inode_bitmap所在的扇区中没有多余的无效位 */
    write_sector(sb.inode_bitmap_lba, buffer, sb.inode_bitmap_sector);

    /***************************************
     * 4 将inode数组初始化并写入sb.inode_table_lba *
     ***************************************/
    /* 准备写inode_table中的第0项,即根目录所在的inode */
    memset(buffer, 0, buffer_size);  // 先清空缓冲区buf
    Inode* i     = (Inode*)buffer;
    i->size      = sb.dir_entry_size * 2;  // .和..
    i->no        = 0;                      // 根目录占inode数组中第0个inode
    i->sector[0] = sb.block_start_lba;     // 由于上面的memset,i_sectors数组的其它元素都初始化为0

    write_sector(sb.inode_table_lba, buffer, sb.inode_table_sector);

    /***************************************
     * 5 将根目录初始化并写入sb.data_start_lba
     ***************************************/
    /* 写入根目录的两个目录项.和.. */
    memset(buffer, 0, buffer_size);
    DirectoryEntry* directory_entry = (DirectoryEntry*)buffer;

    /* 初始化当前目录"." */
    memcpy(directory_entry->file_name, ".", 2);
    directory_entry->inode_number = 0;
    directory_entry->file_type    = FileType::directory;
    directory_entry++;

    /* 初始化当前目录父目录".." */
    memcpy(directory_entry->file_name, "..", 3);
    directory_entry->inode_number = 0;  // 根目录的父目录依然是根目录自己
    directory_entry->file_type    = FileType::directory;

    /* sb.data_start_lba已经分配给了根目录,里面是根目录的目录项 */
    write_sector(sb.block_start_lba, buffer, 1);

    printk("root_dir_lba:0x%x\n", sb.block_start_lba);
    printk("%s format done\n", name);
    printkln("buffer %x", buffer);
    Memory::free(buffer);
}

Inode* Partition::open_inode(uint32_t no)
{
    LOG_LINE();
    ASSERT(no < MAX_FILES_PER_PART);
    if (!inode_list.is_empty())
    {
        for (auto p = &inode_list.front(); p != inode_list.back().next; p = p->next)
        {
            Inode* inode = (Inode*)((uint32_t)p - (uint32_t) & ((Inode*)0)->list_tag);
            if (inode->no == no)
            {
                inode->open_count += 1;
                return inode;
            }
        }
    }

    uint32_t index = no * sizeof(Inode);
    ASSERT(index + sizeof(Inode) <= partition_sector_count * SECTOR_SIZE);
    uint32_t byte_offset = index * sizeof(Inode) + block_start_sector * SECTOR_SIZE;
    Inode*   node        = (Inode*)Memory::malloc_kernel(sizeof(Inode));
    ASSERT(node != nullptr);
    read_byte(byte_offset, node, sizeof(Inode));
    node->open_count = 1;
    inode_list.push_front(node->list_tag);
    return node;
}

void Partition::close_inode(Inode* inode)
{
    LOG_LINE();
    ASSERT(inode != nullptr);
    AtomicGuard gurad;
    ASSERT(inode_list.find(inode->list_tag));
    ASSERT(inode->open_count > 0);
    inode->open_count -= 1;
    if (inode->open_count == 0)
    {
        inode_list.remove(inode->list_tag);
        // to do
        Memory::free_kernel(inode);  // malloc_kernel 应该用malloc_free 释放内存
        //未实现硬盘同步
    }
}

void Partition::read_byte(uint32_t byte_address, void* buffer, uint32_t byte_count)
{
    ASSERT(byte_address + byte_count < partition_sector_count * SECTOR_SIZE);
    uint32_t start_sector                   = byte_address / SECTOR_SIZE;
    uint32_t end_sector                     = (byte_address + byte_count - 1) / SECTOR_SIZE;
    uint32_t sector_count                   = 1 + end_sector - start_sector;
    uint32_t first_block_byte_start_address = byte_address % SECTOR_SIZE;
    uint8_t* sector_buffer                  = (uint8_t*)Memory::malloc(SECTOR_SIZE);
    read_sector(start_sector, sector_buffer, 1);
    ASSERT(sector_count > 0);
    if (sector_count == 1)
    {
        memcpy(buffer, sector_buffer + first_block_byte_start_address, byte_count);
    }
    else
    {
        uint32_t last_block_byte_end_address = (byte_address + byte_count - 1) % SECTOR_SIZE;
        uint8_t* dest                        = (uint8_t*)buffer;
        memcpy(dest, sector_buffer + first_block_byte_start_address, SECTOR_SIZE - first_block_byte_start_address);
        dest += SECTOR_SIZE - first_block_byte_start_address;
        for (uint32_t i = 1; i < sector_count - 1; i++)
        {
            read_sector(start_sector + i, sector_buffer, 1);
            memcpy(dest, sector_buffer, SECTOR_SIZE);
            dest += SECTOR_SIZE;
        }
        read_sector(end_sector, sector_buffer, 1);
        memcpy(dest, sector_buffer, last_block_byte_end_address + 1);
    }
    Memory::free(sector_buffer);
}

void Partition::write_byte(uint32_t byte_address, void* buffer, uint32_t byte_count)
{
    ASSERT(byte_address + byte_count < partition_sector_count * SECTOR_SIZE);
    uint32_t start_sector                   = byte_address / SECTOR_SIZE;
    uint32_t end_sector                     = (byte_address + byte_count - 1) / SECTOR_SIZE;
    uint32_t sector_count                   = 1 + end_sector - start_sector;
    uint32_t first_block_byte_start_address = byte_address % SECTOR_SIZE;
    uint8_t* sector_buffer                  = (uint8_t*)Memory::malloc(SECTOR_SIZE);
    read_sector(start_sector, sector_buffer, 1);
    ASSERT(sector_count > 0);
    if (sector_count == 1)
    {
        memcpy(sector_buffer + first_block_byte_start_address, buffer, byte_count);
        write_sector(start_sector, sector_buffer, 1);
    }
    else
    {
        memcpy(sector_buffer + first_block_byte_start_address, buffer, SECTOR_SIZE - first_block_byte_start_address);
        write_sector(start_sector, sector_buffer, 1);
        uint32_t last_block_byte_end_address = (byte_address + byte_count - 1) % SECTOR_SIZE;
        uint8_t* dest                        = (uint8_t*)buffer + SECTOR_SIZE - first_block_byte_start_address;
        for (uint32_t i = 1; i < sector_count - 1; i++)
        {
            write_sector(start_sector + i, dest, 1);
            dest += SECTOR_SIZE;
        }
        read_sector(end_sector, sector_buffer, 1);
        memcpy(sector_buffer, dest, last_block_byte_end_address + 1);
        write_sector(end_sector, sector_buffer, 1);
    }
    Memory::free(sector_buffer);
}

bool Partition::is_valid()
{
    return valid;
}

void Partition::read_sector(uint32_t lba, void* buffer, uint32_t sector_count)
{
    ASSERT(lba < partition_sector_count);
    ASSERT(lba + sector_count <= partition_lba_base + partition_sector_count);
    IDE::read(my_disk, partition_lba_base + lba, buffer, sector_count);
}

void Partition::write_sector(uint32_t lba, void* buffer, uint32_t sector_count)
{
    ASSERT(lba < partition_sector_count);
    ASSERT(lba + sector_count <= partition_lba_base + partition_sector_count);
    IDE::write(my_disk, partition_lba_base + lba, buffer, sector_count);
}

bool Partition::is_formated()
{
    ASSERT(is_valid());
    return formated;
}

void Partition::print_super_block_info()
{
    SuperBlock* super_block = (SuperBlock*)Memory::malloc(sizeof(SuperBlock));
    ASSERT(sizeof(SuperBlock) == 512);
    read_sector(1, super_block, 1);
    printk("%s info:\n", name);
    printk("magic:%x lba_base:%x sector count:%x\n", super_block->magic, super_block->partition_lba_base,
           super_block->sector_count);
    printk("inode count:%x block bitmap lba:%x block bitmap sector:%x\n", super_block->inode_count,
           super_block->block_bitmap_lba, super_block->block_bitmap_sector);
    printk("inode bitmap lba:%x inodebitmap sector:%x\n", super_block->inode_bitmap_lba,
           super_block->inode_bitmap_sector);
    printk("inode table lba:%x inode table sector:%x data lba %x data sector:%x\n", super_block->inode_table_lba,
           super_block->inode_table_sector, super_block->block_start_lba,
           super_block->sector_count - super_block->block_start_lba);
    printk("block bimap start %x: size:%x, inode bitmap start:%x size:%x\n", block_bitmap.start_address,
           block_bitmap.byte_size, inode_bitmap.start_address, inode_bitmap.byte_size);
    Memory::free(super_block);
}