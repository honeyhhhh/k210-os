#ifndef _TIMER_H
#define _TIMER_H

#include "stddef.h"

// 1000ms = 1s
const uint64_t MSEC_PER_SEC = 1000;

// 时钟中断间隔
static uint64_t INTERVAL = 100000;

// 触发时钟中断计数
static uint64_t TICKS = 0;

static inline uint64_t get_time(void);
static inline uint64_t get_time_v2(void);
void timer_init(void);
void set_next_trigger(void);



#endif