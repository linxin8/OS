#include "kernel/memory.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "lib/macro.h"
#include "lib/math.h"
#include "lib/string.h"
#include "process/process.h"
#include "thread/sync.h"
#include "thread/thread.h"
/***************  位图地址 ********************
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M内存 */
#define MEM_BITMAP_BASE 0xc009a000

#define MEM_BITMAP_MAX 0xc009f000

/* 0xc0000000是内核从虚拟地址3G起.
 * 0x100000意指跨过低端1M内存,使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000

#define PG_P_1 1   // 页表项或页目录项存在属性位
#define PG_P_0 0   // 页表项或页目录项存在属性位
#define PG_RW_R 0  // R/W 属性位值, 读/执行
#define PG_RW_W 2  // R/W 属性位值, 读/写/执行
#define PG_US_S 0  // U/S 属性位值, 系统级，只允许特权级别为 0、 1、 2 的程序访问此页内存，3 特权级程序不被允许。
#define PG_US_U 4  // U/S 属性位值, 用户级，只允许特权级别为 0、 1、 2 的程序访问此页内存，3 特权级程序不被允许。

#define DESC_CNT 7  // 内存块描述符个数

//物理内存池
struct PhysicalAddressPool
{
    Bitmap   bitmap;         //记录物理内存使用情况
    void*    start_address;  //物理起始地址
    uint32_t size;           //物理内存大小
    // Lock     lock;
};

//内存池
struct MemoryPool
{
    PhysicalAddressPool physical_address_pool;
    VirtualAddressPool  virtual_address_pool;
};

struct Arena
{
    MemoryBlockDescript* descript;  // 此arena关联的MemoryBlockDescript
    /* isPage为ture时,count表示的是页框数。
     * 否则count表示空闲memory block数量 */
    bool     isPage;
    uint32_t count;
};

//内核内存池
MemoryPool kernel_memory_pool;
//用户内存池
MemoryPool user_memory_pool;

//一共支持7中类型的block
MemoryBlockDescript memory_block_decript[7];

struct Area
{
    MemoryBlockDescript* descript;
    /* isPage为ture时,count表示的是页框数。
     * 否则count表示空闲memory block数量 */
    bool isPage;
    //空闲的block或page数目
    uint32_t count;
};

typedef ListElement MemoryBlock;

void init_memory_pool(uint32_t total_memory)
{
    uint32_t page_table_size = PAGE_SIZE * 256;             // 256个页表
    uint32_t used_memory     = page_table_size + 0x100000;  //已用的内存，低1MB内存已被占用
    uint32_t free_memory     = total_memory - used_memory;  //可用的内存
    uint32_t all_free_page   = free_memory / PAGE_SIZE;     //可用的内存页数目

    uint32_t kernel_free_page = all_free_page / 2;  //内核可用的内存页数目

    uint32_t user_free_page = all_free_page - kernel_free_page;  //用户可用的内核内存页数目

    uint32_t kbitmap_length = kernel_free_page / 8;  //内核可用的内存页位图长度
    uint32_t ubitmap_length = user_free_page / 8;    // 用户可用的内存页位图长度

    uint32_t k_start = used_memory;                             //内核可用的内存起始地址
    uint32_t u_start = k_start + kernel_free_page * PAGE_SIZE;  //用户可用的内存起始地址

    //设置物理起始地址
    kernel_memory_pool.physical_address_pool.start_address = (void*)k_start;
    user_memory_pool.physical_address_pool.start_address   = (void*)u_start;

    //设置可用物理内存大小
    kernel_memory_pool.physical_address_pool.size = kernel_free_page * PAGE_SIZE;
    user_memory_pool.physical_address_pool.size   = user_free_page * PAGE_SIZE;

    //初始化物理内存bitmap
    kernel_memory_pool.physical_address_pool.bitmap.init(kbitmap_length, (uint8_t*)MEM_BITMAP_BASE);
    user_memory_pool.physical_address_pool.bitmap.init(ubitmap_length, (uint8_t*)(MEM_BITMAP_BASE + kbitmap_length));

    //初始化内核虚拟内存
    kernel_memory_pool.virtual_address_pool.start_address = (uint8_t*)K_HEAP_START;
    kernel_memory_pool.virtual_address_pool.bitmap.init(kbitmap_length,
                                                        (uint8_t*)(MEM_BITMAP_BASE + kbitmap_length + ubitmap_length));

    //位图大小不能超过内存划定范围
    ASSERT(MEM_BITMAP_BASE + kbitmap_length + ubitmap_length + kbitmap_length < MEM_BITMAP_MAX);

    // lock_init(&kernel_pool.lock);
    // lock_init(&user_pool.lock);

    //输出内存池信息
    printk("kernel bitmap start address %x, physical start address %x\n",
           kernel_memory_pool.physical_address_pool.bitmap.start_address,
           kernel_memory_pool.physical_address_pool.start_address);

    printk("user bitmap start address %x, physical start address %x\n",
           user_memory_pool.physical_address_pool.bitmap.start_address,
           user_memory_pool.physical_address_pool.start_address);
}

void Memory::init_block_descript(MemoryBlockDescript* descript)
{
    for (int i = 0; i < 7; i++)
    {
        uint32_t block_size    = 16 << i;
        descript[i].block_size = block_size;
        descript[i].free_list  = List();
    }
}

void Memory::init()
{
    printkln("memory init start");
    kernel_memory_pool = MemoryPool();
    user_memory_pool   = MemoryPool();
    //参照load.asm，读取的内存数保存在0xb00中
    uint32_t memory_bytes_total = (*(uint32_t*)(0xb00));
    init_memory_pool(memory_bytes_total);
    init_block_descript(memory_block_decript);
    printkln("memory init done");
}

void* malloc_kernel_virutal_page(uint32_t count)
{
    int32_t index = kernel_memory_pool.virtual_address_pool.bitmap.scan(count);
    if (index == -1)
    {
        return nullptr;
    }
    kernel_memory_pool.virtual_address_pool.bitmap.fill(index, count, true);
    return (uint8_t*)kernel_memory_pool.virtual_address_pool.start_address + index * PAGE_SIZE;
}

void* malloc_user_virutal_page(PCB* pcb, uint32_t count)
{
    ASSERT(Thread::is_user_thread(pcb));
    int32_t index = pcb->user_virutal_address_pool.bitmap.scan(count);
    if (index == -1)
    {
        return nullptr;
    }
    pcb->user_virutal_address_pool.bitmap.fill(index, count, true);
    void* address = (uint8_t*)pcb->user_virutal_address_pool.start_address + index * PAGE_SIZE;
    ASSERT((uint32_t)address < (0xc0000000 - PAGE_SIZE));
    return address;
}

void* malloc_one_kernel_physical_page()
{
    int32_t index = kernel_memory_pool.physical_address_pool.bitmap.scan(1);
    if (index == -1)
    {
        return nullptr;
    }
    ASSERT(!kernel_memory_pool.physical_address_pool.bitmap.test(index));
    kernel_memory_pool.physical_address_pool.bitmap.set(index, true);
    return (uint8_t*)kernel_memory_pool.physical_address_pool.start_address + index * PAGE_SIZE;
}

void* malloc_one_user_physical_page()
{
    int32_t index = user_memory_pool.physical_address_pool.bitmap.scan(1);
    if (index == -1)
    {
        return nullptr;
    }
    ASSERT(!user_memory_pool.physical_address_pool.bitmap.test(index));
    user_memory_pool.physical_address_pool.bitmap.set(index, true);
    return (uint8_t*)user_memory_pool.physical_address_pool.start_address + index * PAGE_SIZE;
}

bool is_pde_exist(uint32_t* pde)
{
    return *pde & PG_P_1;
}

bool is_pte_exist(uint32_t* pte)
{
    return *pte & PG_P_1;
}

//获取虚地址的PDE的索引
uint32_t get_pde_index(void* virtual_address)
{
    return ((uint32_t)virtual_address & 0xffc00000) >> 22;  //虚地址前10位为pte索引
}

//获取虚地址的PTE的索引
uint32_t get_pte_index(void* virtual_address)
{
    return ((uint32_t)virtual_address & 0x003ff000) >> 12;  //虚地址中间10位为pte索引
}

//获取虚地址的PTE的虚地址
void* get_pte_pointer(void* virtual_address)
{
    uint32_t base             = 0xffc00000;  //前10位设置为最后一个页目录，指向页目录起始处
    uint32_t pde_index        = (uint32_t)get_pde_index(virtual_address);
    uint32_t pte_index        = get_pte_index(virtual_address);
    uint32_t pte_virtual_addr = base + (pde_index << 12) + (pte_index * 4);
    return (void*)pte_virtual_addr;
}

void* get_pde_pointer(void* virtual_address)
{  //获取虚地址的PDE的虚地址
   //前10和中间10位设置为最后一个页目录，指向页目录起始处
    void* pde_virtual_addr = (void*)((0xfffff000) + get_pde_index(virtual_address) * 4);
    return pde_virtual_addr;
}

void map_page(void* physical_page_address, void* virtual_page_address)
{
    uint32_t* pde = (uint32_t*)get_pde_pointer(virtual_page_address);
    uint32_t* pte = (uint32_t*)get_pte_pointer(virtual_page_address);
    if (is_pde_exist(pde))
    {
        ASSERT(!is_pte_exist(pte));
        *pte = (uint32_t)physical_page_address | PG_US_U | PG_RW_W | PG_P_1;
    }
    else
    {
        //先创建pde
        uint32_t pde_physical_address = (uint32_t)malloc_one_kernel_physical_page();
        *pde                          = pde_physical_address | PG_US_U | PG_RW_W | PG_P_1;
        // pte & 0xfffff000是pde_physical_address的虚地址
        memset((void*)((uint32_t)pte & 0xfffff000), 0, PAGE_SIZE);
        ASSERT(!is_pte_exist(pte));
        //创建pte
        *pte = (uint32_t)physical_page_address | PG_US_U | PG_RW_W | PG_P_1;
    }
    // asm volatile("invlpg %0" ::"m"(*(char*)virtual_page_address) : "memory");  //更新tlb
    Process::activate_page_directory(Thread::get_current_pcb());  //更新tlb,i386不支持invlpg
}

void* Memory::malloc_user_page(uint32_t count)
{
    AtomicGuard guard;
    PCB*        pcb = Thread::get_current_pcb();
    ASSERT(Thread::is_pcb_valid(pcb));
    void* virutal_page = malloc_user_virutal_page(pcb, count);
    if (virutal_page == nullptr)
    {
        printkln("virutal_page is nullptr");
        return nullptr;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        void* physical_page = malloc_one_user_physical_page();
        if (physical_page == nullptr)
        {
            printkln("malloc one user physical page failed\n");
            return nullptr;
        }
        map_page(physical_page, (void*)(((uint32_t)virutal_page) + PAGE_SIZE * i));
    }
    return virutal_page;
}

void* Memory::malloc_kernel_page(uint32_t count)
{
    AtomicGuard guard;
    ASSERT(count > 0 && count < 3040);
    void* virutal_page = malloc_kernel_virutal_page(count);
    if (virutal_page == nullptr)
    {
        return nullptr;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        void* physical_page = malloc_one_kernel_physical_page();
        if (physical_page == nullptr)
        {
            printkln("malloc one kernel physical page failed");
            return nullptr;
        }
        map_page(physical_page, (void*)(((uint32_t)virutal_page) + PAGE_SIZE * i));
    }
    return virutal_page;
}

//从内核堆中分配内存
void* Memory::malloc_kernel(uint32_t size)
{
    AtomicGuard guard;
    PCB*        pcb    = Thread::get_current_pcb();
    auto        backup = pcb->pgd;  //暂时让系统认为pcb是内核线程，达到分配内核内存的目的
    pcb->pgd           = nullptr;
    void* p            = malloc(size);
    pcb->pgd           = backup;
    return p;
}

//从内核堆中释放内存
void Memory::free_kernel(void* p)
{
    AtomicGuard guard;
    PCB*        pcb    = Thread::get_current_pcb();
    auto        backup = pcb->pgd;  //暂时让系统认为pcb是内核线程，达到释放内核内存的目的
    pcb->pgd           = nullptr;
    free(p);
    pcb->pgd = backup;
}

void* Memory::malloc(uint32_t size)
{
    AtomicGuard guard;
    bool        is_kernel = Thread::is_current_kernel_thread();
    if (size > 1024)
    {
        uint32_t count = div_round_up(size + sizeof(Arena), PAGE_SIZE);  //除了分配用户内存，还需要分配area内存
        Area* area = (Area*)(is_kernel ? malloc_kernel_page(count) : malloc_user_page(count));
        if (area == nullptr)
        {
            return nullptr;
        }
        area->isPage   = true;
        area->count    = count;
        area->descript = nullptr;
        void* block    = (void*)((uint32_t)area + sizeof(Area));
        memset(block, 0, area->count * PAGE_SIZE - sizeof(Area));
        return block;
    }
    else
    {
        MemoryBlockDescript* descript =
            is_kernel ? memory_block_decript : Thread::get_current_pcb()->user_block_descript;
        for (; descript->block_size < size; descript++) {}
        if (descript->free_list.is_empty())
        {
            Area* area = (Area*)(is_kernel ? malloc_kernel_page(1) : malloc_user_page(1));
            if (area == nullptr)
            {
                return nullptr;
            }
            area->isPage   = false;
            area->descript = descript;
            area->count    = (PAGE_SIZE - sizeof(Area)) / area->descript->block_size;
            for (uint32_t i = 0; i < area->count; i++)
            {
                MemoryBlock* block = (MemoryBlock*)((uint32_t)area + sizeof(Area) + i * area->descript->block_size);
                ASSERT(!area->descript->free_list.find(block));
                block->previous = nullptr;
                block->next     = nullptr;
                area->descript->free_list.push_back(block);
            }
        }
        MemoryBlock* block = descript->free_list.pop_front();
        Area*        area  = (Area*)((uint32_t)block & 0xfffff000);
        ASSERT(area->count > 0);
        area->count--;
        memset(block, 0, area->descript->block_size);
        return block;
    }
}

//获取虚拟地址对应的物理地址
void* Memory::get_phsical_address_by_virtual_address(void* virtual_address)
{
    uint32_t* pte = (uint32_t*)get_pte_pointer(virtual_address);
    ASSERT(is_pte_exist(pte));
    /* (*pte)的值是页表所在的物理页框地址，去掉其低 12 位的页表项属性+虚拟地址 vaddr 的低 12 位 */
    return (void*)((*pte & 0xfffff000) + (((uint32_t)virtual_address) & 0x00000fff));
}

void free_user_page(void* virtual_addr, uint32_t count)
{
    uint32_t vaddr = (uint32_t)virtual_addr;
    ASSERT(vaddr % PAGE_SIZE == 0);
    uint32_t p_start_address = (uint32_t)user_memory_pool.physical_address_pool.start_address;
    uint32_t v_start_address = (uint32_t)Thread::get_current_pcb()->user_virutal_address_pool.start_address;
    // uint32_t v_start_address = (uint32_t)user_memory_pool.virtual_address_pool.start_address;
    for (uint32_t i = 0; i < count; i++)
    {
        //释放实页
        uint32_t paddr = (uint32_t)Memory::get_phsical_address_by_virtual_address((void*)vaddr);
        ASSERT(paddr % PAGE_SIZE == 0);
        ASSERT(paddr >= p_start_address);
        uint32_t p_index = (paddr - p_start_address) / PAGE_SIZE;
        ASSERT(user_memory_pool.physical_address_pool.bitmap.test(p_index));
        user_memory_pool.physical_address_pool.bitmap.set(p_index, false);
        //释放pte，为了简化操作，pde不释放，等进程结束了回收pde
        uint32_t* pte = (uint32_t*)get_pte_pointer((void*)vaddr);
        *pte &= ~PG_P_1;  // 将页表项pte的P位置0
        // asm volatile("invlpg %0" ::"m"(vaddr) : "memory");  //更新tlb
        Process::activate_page_directory(Thread::get_current_pcb());  //更新tlb,i386不支持invlpg

        //释放虚页
        ASSERT(vaddr >= v_start_address);
        uint32_t v_index = (vaddr - v_start_address) / PAGE_SIZE;
        ASSERT(Thread::get_current_pcb()->user_virutal_address_pool.bitmap.test(v_index));
        Thread::get_current_pcb()->user_virutal_address_pool.bitmap.set(v_index, false);
        vaddr = vaddr + PAGE_SIZE;
    }
}

void free_kernel_page(void* virtual_addr, uint32_t count)
{
    uint32_t vaddr = (uint32_t)virtual_addr;
    ASSERT(count > 0 && (uint32_t)vaddr % PAGE_SIZE == 0);
    uint32_t p_start_address = (uint32_t)kernel_memory_pool.physical_address_pool.start_address;
    uint32_t v_start_address = (uint32_t)kernel_memory_pool.virtual_address_pool.start_address;
    for (uint32_t i = 0; i < count; i++)
    {
        //释放实页
        uint32_t paddr = (uint32_t)Memory::get_phsical_address_by_virtual_address((void*)vaddr);
        ASSERT(paddr % PAGE_SIZE == 0);
        ASSERT(paddr >= p_start_address);
        uint32_t p_index = (paddr - p_start_address) / PAGE_SIZE;
        ASSERT(kernel_memory_pool.physical_address_pool.bitmap.test(p_index));
        kernel_memory_pool.physical_address_pool.bitmap.set(p_index, false);

        //释放pte，为了简化操作，pde不释放，等进程结束了回收pde
        uint32_t* pte = (uint32_t*)get_pte_pointer((void*)vaddr);
        *pte &= ~PG_P_1;  // 将页表项pte的P位置0
        // asm volatile("invlpg %0" ::"m"(vaddr) : "memory");  //更新tlb
        Process::activate_page_directory(Thread::get_current_pcb());  //更新tlb,i386不支持invlpg

        //释放虚页
        ASSERT(vaddr >= v_start_address);
        uint32_t v_index = (vaddr - v_start_address) / PAGE_SIZE;
        ASSERT(kernel_memory_pool.virtual_address_pool.bitmap.test(v_index));
        kernel_memory_pool.virtual_address_pool.bitmap.set(v_index, false);
        vaddr = vaddr + PAGE_SIZE;
    }
}

void Memory::free(void* vaddr)
{
    AtomicGuard guard;
    ASSERT(vaddr != nullptr);
    bool         is_kernel = Thread::is_current_kernel_thread();
    MemoryBlock* block     = (MemoryBlock*)vaddr;
    Area*        area      = (Area*)((uint32_t)block & 0xfffff000);

    if (area->isPage)
    {
        if (is_kernel)
        {
            free_kernel_page(area, area->count);
        }
        else
        {
            free_user_page(area, area->count);
        }
    }
    else
    {
        area->count++;
        block->previous = nullptr;
        block->next     = nullptr;
        ASSERT(!area->descript->free_list.find(block));
        area->descript->free_list.push_back(block);
        uint32_t block_per_page = (PAGE_SIZE - sizeof(Area)) / area->descript->block_size;
        ASSERT(area->count <= block_per_page);
        if (area->count == block_per_page)
        {
            for (uint32_t i = 0; i < block_per_page; i++)
            {
                MemoryBlock* block = (MemoryBlock*)((uint32_t)area + sizeof(Area) + i * area->descript->block_size);
                ASSERT(area->descript->free_list.find(block));
                block->remove_from_list();
            }
            if (is_kernel)
            {
                free_kernel_page(area, 1);
            }
            else
            {
                free_user_page(area, 1);
            }
        }
    }
}

void Memory::malloc_physical_page_for_virtual_page(bool is_kernel, void* virtual_page)
{
    PCB* pcb = Thread::get_current_pcb();
    // bool     is_kernel = Thread::is_current_kernel_thread();
    uint32_t vaddr = (uint32_t)virtual_page;
    ASSERT((vaddr & 0xfff) == 0);
    if (is_kernel)
    {
        ASSERT(vaddr >= (uint32_t)kernel_memory_pool.virtual_address_pool.start_address);
        uint32_t index = (vaddr - (uint32_t)kernel_memory_pool.virtual_address_pool.start_address) / PAGE_SIZE;
        ASSERT(!kernel_memory_pool.virtual_address_pool.bitmap.test(index));
        kernel_memory_pool.virtual_address_pool.bitmap.set(index, true);
    }
    else
    {
        ASSERT(vaddr >= (uint32_t)pcb->user_virutal_address_pool.start_address);
        uint32_t index = (vaddr - (uint32_t)pcb->user_virutal_address_pool.start_address) / PAGE_SIZE;
        ASSERT(!pcb->user_virutal_address_pool.bitmap.test(index));
        pcb->user_virutal_address_pool.bitmap.set(index, true);
    }
    void* physical_page = is_kernel ? malloc_one_kernel_physical_page() : malloc_one_user_physical_page();
    if (physical_page != nullptr)
    {
        map_page(physical_page, virtual_page);
    }
}