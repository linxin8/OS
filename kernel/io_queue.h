#pragma once

#include "lib/stdint.h"
#include "thread/sync.h"

#define IO_QUEUE_BUFF_SIZE 100
// 环形队列
class IOQueue
{
public:
    IOQueue();
    uint8_t  pop_front();
    void     push_back(uint8_t byte);
    uint32_t get_length();
    bool     is_full();
    bool     is_empty();

private:
    uint8_t  buff[IO_QUEUE_BUFF_SIZE];  // 缓冲区大小
    uint32_t head;                      // 队首,数据往队首处写入
    uint32_t tail;                      // 队尾,数据从队尾处读出
};
