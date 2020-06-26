#include "thread/thread.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "kernel/log.h"
#include "lib/debug.h"
#include "lib/macro.h"
#include "lib/stdint.h"
#include "lib/string.h"
#include "process/process.h"
#include "process/tss.h"
#include "thread/sync.h"

#define PCB_STACK_MAGIC 0x01234567U
PCB* main_thread;
PCB* idle_thread;

struct ThreadPool
{
    List ready_list;
    List blocked_list;
    List running_list;
    List deid_list;
    Lock lock;
};
ThreadPool thread_pool;

//获取当前进程的PCB
PCB* Thread::get_current_pcb()
{
    uint32_t esp = 0;
    asm("mov %%esp, %0" : "=g"(esp));
    return (PCB*)(esp & 0xfffff000);  // pcb在页的起始位置
}

pid_t Thread::alloc_pid()
{
    AtomicGuard  guard;
    static pid_t pid = 0;
    return ++pid;
}

//判断当前线程/进程是否为内核线程
bool Thread::is_current_kernel_thread()
{
    return get_current_pcb()->pgd == nullptr;
}

//判断当前线程/进程是否为用户线程
bool Thread::is_current_user_thread()
{
    return get_current_pcb()->pgd != nullptr;
}

//检测当前的pcb是否合法
bool Thread::is_current_pcb_valid()
{
    return is_pcb_valid(get_current_pcb());
}

//检测pcb是否合法
bool Thread::is_pcb_valid(PCB* pcb)
{
    return ((((uint32_t)pcb) & 0xfffu) == 0) && pcb->stack_magic == PCB_STACK_MAGIC;
}

PCB* Thread::get_pcb_by_semaphore_tag(ListElement* semaphore_tag)
{
    ASSERT(semaphore_tag != nullptr);
    PCB* pcb = (PCB*)((uint32_t)semaphore_tag - (uint32_t) & ((PCB*)0)->semaphore_tag);
    ASSERT(Thread::is_pcb_valid(pcb));
    return pcb;
}

PCB* Thread::get_pcb_by_thread_list_tag(ListElement* thread_list_tag)
{
    ASSERT(thread_list_tag != nullptr);
    PCB* pcb = (PCB*)((uint32_t)thread_list_tag - (uint32_t) & ((PCB*)0)->thread_list_tag);
    ASSERT(Thread::is_pcb_valid(pcb));
    return pcb;
}

void Thread::unblock_thread(PCB* thread)
{
    LockGuard guard(thread_pool.lock);
    ASSERT(thread->status == TaskStatus::blocked || thread->status == TaskStatus::hanging ||
           thread->status == TaskStatus::waiting);
    if (thread->status != TaskStatus::ready)
    {
        ASSERT(!thread_pool.ready_list.find(thread->thread_list_tag));
        if (thread_pool.ready_list.find(thread->thread_list_tag))
        {
            PANIC("thread is in ready list\n");
        }
        thread->thread_list_tag.remove_from_list();
        thread_pool.ready_list.push_back(thread->thread_list_tag);
        thread->status = TaskStatus::ready;
    }
}

void schedule(TaskStatus next_status)
{
    AtomicGuard guard;
    PCB*        pcb         = Thread::get_current_pcb();
    PCB*        next_thread = nullptr;

    {  //对线程池的操作加锁
        LockGuard guard(thread_pool.lock);
        pcb->thread_list_tag.remove_from_list();
        pcb->status = next_status;
        switch (next_status)
        {
            case TaskStatus::ready: thread_pool.ready_list.push_back(pcb->thread_list_tag); break;
            case TaskStatus::died: thread_pool.deid_list.push_back(pcb->thread_list_tag); break;
            case TaskStatus::hanging: thread_pool.blocked_list.push_back(pcb->thread_list_tag); break;
            case TaskStatus::running: ASSERT(false); break;  //错误的状态
            case TaskStatus::waiting: thread_pool.blocked_list.push_back(pcb->thread_list_tag); break;
            case TaskStatus::blocked: thread_pool.blocked_list.push_back(pcb->thread_list_tag); break;
            default: ASSERT(false); break;  //错误的状态
        }
        ASSERT(!thread_pool.ready_list.is_empty());
        next_thread         = Thread::get_pcb_by_thread_list_tag(thread_pool.ready_list.pop_front());
        next_thread->ticks  = next_thread->priority;
        next_thread->status = TaskStatus::running;
        thread_pool.running_list.push_back(next_thread->thread_list_tag);
    }

    /* 激活该进程或线程的页表 */
    Process::activate_page_directory(next_thread);
    /* 内核线程特权级本身就是0,处理器进入中断时并不会从tss中获取0特权级栈地址,故不需要更新esp0 */
    // if (Thread::is_kernel_thread(pcb))
    if (Thread::is_user_thread(next_thread))
    {
        // 更新该用户进程的esp0,设置此进程被中断时的0级栈底
        TSS::update_esp0(next_thread);
    }
    Log::thread_switch(pcb->name, next_thread->name);
    switch_to(pcb, next_thread);
}

void Thread::init_pcb(PCB* pcb, const char* name, int priority)
{
    memset(pcb, 0, sizeof(PCB));
    strcpy(pcb->name, name);
    if (pcb == main_thread)
    {
        /* 由于把main函数也封装成一个线程,并且它一直是运行的,故将其直接设为TASK_RUNNING */
        pcb->status = TaskStatus::running;
    }
    else
    {
        pcb->status = TaskStatus::ready;
    }

    pcb->pid           = alloc_pid();
    pcb->priority      = priority;
    pcb->ticks         = priority;
    pcb->elapsed_ticks = 0;
    pcb->pgd           = nullptr;
    pcb->self_kstack   = (uint32_t*)((uint32_t)pcb + PAGE_SIZE);
    pcb->stack_magic   = PCB_STACK_MAGIC;

    /* 标准输入输出先空出来 */
    pcb->file_table[0] = 0;  // stdin
    pcb->file_table[1] = 1;  // stdout
    pcb->file_table[2] = 2;  // stderr
    /* 其余的全置为-1 */
    for (int i = 3; i < 8; i++)
    {
        pcb->file_table[i] = -1;
    }
    pcb->work_directory_inode = 0;   // 以根目录做为默认工作路径
    pcb->parent_pid           = -1;  // -1表示没有父进程
}

//创建的进程以这个函数作为入口，进入真正的函数
void thread_entry(ThreadCallbackFunction_t function, void* function_arg)
{                         //此处已经是新的进程的代码段
    Interrupt::enable();  //在进入新进程时，中断在之前可能已经被关闭，所以在此确保中断已打开
    function(function_arg);
}

void idle(void* arg)
{
    UNUSED(arg);
    printk("idle thread start\n");
    while (true)
    {
        Thread::yield();
        //执行hlt时必须要保证目前处在开中断的情况下
        asm volatile("sti; hlt" : : : "memory");
    }
}

//创建内核线程，并加入就绪队列
PCB* Thread::create_thread(const char* name, int priority, ThreadCallbackFunction_t function, void* function_arg)
{
    /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
    PCB* pcb = (PCB*)Memory::malloc_kernel_page(1);
    init_pcb(pcb, name, priority);
    pcb->self_kstack -= sizeof(InterruptStack);
    pcb->self_kstack -= sizeof(ThreadStack);
    ThreadStack* stack  = (ThreadStack*)pcb->self_kstack;
    stack->eip          = thread_entry;
    stack->function     = function;
    stack->function_arg = function_arg;
    stack->ebp          = 0;
    stack->ebx          = 0;
    stack->esi          = 0;
    stack->edi          = 0;
    LockGuard gurad(thread_pool.lock);
    thread_pool.ready_list.push_back(pcb->thread_list_tag);
    return pcb;
}

void Thread::yield()
{
    schedule(TaskStatus::ready);
}

bool Thread::is_kernel_thread(PCB* pcb)
{
    ASSERT(is_pcb_valid(pcb));
    return pcb->pgd == nullptr;
}

bool Thread::is_user_thread(PCB* pcb)
{
    ASSERT(is_pcb_valid(pcb));
    return pcb->pgd != nullptr;
}

void Thread::block_current_thread()
{
    schedule(TaskStatus::blocked);
}
void Thread::init()
{
    printkln("thread init start");
    thread_pool = ThreadPool();
    main_thread = get_current_pcb();  // 0xc009e000
    ASSERT((uint32_t)main_thread == 0xc009e000);
    init_pcb(main_thread, "main", 32);
    thread_pool.running_list.push_back(main_thread->thread_list_tag);
    idle_thread = create_thread("idle", 32, &idle, nullptr);
    printkln("thread init done");
}

void Thread::insert_ready_thread(PCB* pcb)
{
    AtomicGuard gurad;
    ASSERT(!thread_pool.ready_list.find(pcb->thread_list_tag));
    thread_pool.ready_list.push_front(pcb->thread_list_tag);
}