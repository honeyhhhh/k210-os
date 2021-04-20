#ifndef _TIMER_H
#define _TIMER_H

#include "stddef.h"

// QEMU_CLOCK_FREQ
const uint64_t QEMU_CLOCK_FREQ = 10000000;

// 1000ms = 1s
const uint64_t MSEC_PER_SEC = 1000;

// 时钟中断间隔
static uint64_t INTERVAL = 100000;

// 触发时钟中断计数 < cycle
static uint64_t TICKS = 0;      

// 每秒触发时钟中断次数?   对于QEMU，时钟频率10MHz，每过1s，mtime增加10 000 000，也就是100ns，如果设置时间间隔100 000，每秒将发生100次中断
const uint64_t TICKS_PER_SEC = 100;


static inline uint64_t get_time(void);
uint64_t get_time_ms(void);
static inline uint64_t get_time_v2(void);
static inline uint64_t get_cycle(void);
void timer_init(void);
void set_next_trigger(void);



#endif