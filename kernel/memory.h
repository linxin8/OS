#pragma once
#include "kernel/bitmap.h"
#include "kernel/list.h"

//一页内存的大小
#define PAGE_SIZE 4096

struct VirtualAddressPool
{
    Bitmap bitmap;         //记录虚拟地址使用情况
    void*  start_address;  //虚拟起始地址
};

//用于描述一种block类型的内存管理
struct MemoryBlockDescript
{
    //当前block的大小
    uint32_t block_size;
    //可用的block组成的list
    List free_list;
};

namespace Memory
{
    void  init();
    void* get_phsical_address_by_virtual_address(void* vaddr);
    void* malloc_kernel_page(uint32_t count);
    void* malloc_user_page(uint32_t count);
    //为虚页分配实页
    void  malloc_physical_page_for_virtual_page(bool is_kernel, void* virtual_page);
    void* malloc(uint32_t size);
    void  free(void* vaddr);
    void  init_block_descript(MemoryBlockDescript* descript);
    void* malloc_kernel(uint32_t size);
    void  free_kernel(void* p);
}  // namespace Memory
