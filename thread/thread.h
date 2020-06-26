#pragma once
#include "kernel/list.h"
#include "kernel/memory.h"
#include "lib/stdint.h"

using ThreadCallbackFunction_t = void (*)(void*);
typedef int16_t pid_t;
#define MAX_FILES_OPEN_PER_THREAD 8

enum class TaskStatus : uint32_t
{
    running,
    ready,
    blocked,
    waiting,
    hanging,
    died
};

/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
struct ThreadStack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    /* 线程第一次执行时,eip指向待调用的函数kernel_thread
    其它时候,eip是指向switch_to的返回地址*/
    void (*eip)(ThreadCallbackFunction_t func, void* func_arg);

    /*****   以下仅供第一次被调度上cpu时使用   ****/

    /* 参数unused_ret只为占位置充数为返回地址 */
    void*                    unused_retaddr;
    ThreadCallbackFunction_t function;      // 由Kernel_thread所调用的函数名
    void*                    function_arg;  // 由Kernel_thread所调用的函数所需的参数
};

/* 进程或线程的pcb,程序控制块 */
struct PCB
{
    uint32_t*  self_kstack;  // 各内核线程都用自己的内核栈
    pid_t      pid;
    TaskStatus status;
    char       name[32];  //进程名称
    uint8_t    priority;
    uint8_t    ticks;  // 每次在处理器上执行的时间嘀嗒数
                       /* 此任务自上cpu运行后至今占用了多少cpu嘀嗒数,
                        * 也就是此任务执行了多久*/
    uint32_t elapsed_ticks;
    //线程的信号量标记
    ListElement semaphore_tag;
    //线程队列标记
    ListElement         thread_list_tag;
    uint32_t*           pgd;                        // 进程页表的虚拟地址,在内核线程中为nullptr
    VirtualAddressPool  user_virutal_address_pool;  // 用户进程的虚拟地址
    MemoryBlockDescript user_block_descript[7];     // 用户进程内存块描述符
    int32_t             file_table[MAX_FILES_OPEN_PER_THREAD];  // 已打开文件数组
    uint32_t            work_directory_inode;                   // 进程所在的工作目录的inode编号
    pid_t               parent_pid;                             // 父进程pid
    int8_t              exit_status;                            // 进程结束时自己调用exit传入的参数
    uint32_t            stack_magic;  // 用这串数字做栈的边界标记,用于检测栈的溢出
};

namespace Thread
{
    void init();
    PCB* get_current_pcb();
    PCB* get_pcb_by_semaphore_tag(ListElement* semaphore_tag);
    PCB* get_pcb_by_thread_list_tag(ListElement* thread_list_tag);
    bool is_current_pcb_valid();
    bool is_current_user_thread();
    bool is_kernel_thread(PCB* pcb);
    bool is_user_thread(PCB* pcb);
    bool is_pcb_valid(PCB* pcb);
    bool is_current_kernel_thread();
    void block_current_thread();
    void unblock_thread(PCB* thread);
    void init_pcb(PCB* pcb, const char* name, int priority);
    //切换当前的线程
    void yield();
    PCB* create_thread(const char* name, int priority, ThreadCallbackFunction_t function, void* function_arg);
    //向file table中插入已打开的文件标识符
    pid_t alloc_pid();
    void  insert_ready_thread(PCB* pcb);
};  // namespace Thread
