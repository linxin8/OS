#pragma once

#include "lib/stdint.h"

//中断处理函数签名，为了支持泛型，使用void*
using InterruptHandler = void*;
typedef void* Syscall_t;

//中断状态
enum class InterruptStatus : uint32_t
{
    off,  // 中断关闭
    on    // 中断打开
};

//中断发生后，栈中的结构
struct InterruptStack
{
    //以下是手动压栈的内容，参考interrupt.asm
    uint32_t no;  //中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;  // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // 以下由cpu从低特权级进入高特权级时压入的内容
    uint32_t error_code;  //中断错误码
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void*    esp;
    uint32_t ss;
};

//保证变量作用域内为原子操作
class AtomicGuard
{
public:
    AtomicGuard();
    ~AtomicGuard();

private:
    InterruptStatus old_status;
};

namespace Interrupt
{
    void            init();
    InterruptStatus get_status();
    void            set_status(InterruptStatus status);
    void            enable();
    void            disable();
    void            register_interrupt_handler(uint8_t no, InterruptHandler function);
    bool            is_enabled();
    bool            is_disabled();
};  // namespace Interrupt
