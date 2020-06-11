#include "kernel/timer.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "lib/macro.h"
#include "lib/math.h"
#include "thread/thread.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT 0x40
#define COUNTER0_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

#define MSECONDS_PER_INTERRUPT (1000 / IRQ0_FREQUENCY)

uint32_t ticks;  // ticks是内核自中断开启以来总共的嘀嗒数

/* 把操作的计数器counter_no、读写锁属性rwl、计数器模式counter_mode写入模式控制寄存器并赋予初始值counter_value */
void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value)
{
    /* 往控制字寄存器端口0x43中写入控制字 */
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    /* 先写入counter_value的低8位 */
    outb(counter_port, (uint8_t)counter_value);
    /* 再写入counter_value的高8位 */
    outb(counter_port, (uint8_t)counter_value >> 8);
}

/* 时钟的中断处理函数 */
void timer_interrupt_handler()
{
    PCB* current_thread = Thread::get_current_pcb();
    ASSERT(Thread::is_pcb_valid(current_thread));  // 检查栈是否溢出
    current_thread->elapsed_ticks++;               // 记录此线程占用的cpu时间嘀
    ticks++;  //从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数
    if (current_thread->ticks == 0)
    {  // 若进程时间片用完就开始调度新的进程上cpu
        Thread::yield();
    }
    else
    {  // 将当前进程的时间片-1
        current_thread->ticks--;
    }
}

// 以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式
void ticks_to_sleep(uint32_t sleep_ticks)
{
    uint32_t start_tick = ticks;
    while (ticks - start_tick < sleep_ticks)
    {
        // 若间隔的ticks数不够便让出cpu
        Thread::yield();
    }
}

// 以毫秒为单位的sleep   1秒= 1000毫秒
void Timer::sleep(uint32_t msecond)
{
    uint32_t sleep_ticks = div_round_up(msecond, MSECONDS_PER_INTERRUPT);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}

/* 初始化PIT8253 */
void Timer::init()
{
    printkln("timer init start");
    /* 设置8253的定时周期,也就是发中断的周期 */
    frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    Interrupt::register_interrupt_handler(0x20, (InterruptHandler)timer_interrupt_handler);  //设置时钟中断
    ticks = 0;
    printkln("timer init done");
}
