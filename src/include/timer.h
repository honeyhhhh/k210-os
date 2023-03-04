#ifndef _TIMER_H
#define _TIMER_H

#include "stddef.h"

// 不要在头文件 定义变量， 否则会出现multiple definition of `xxx';
// 可以声明，也可以extern
// 用static修饰一个变量或者函数时，表示该名字只能在文件内部使用，文件外部是看不到该名字的。不能使用extern

// CLOCK_FREQ
extern const uint64_t QEMU_CLOCK_FREQ;
extern const uint64_t K210_CLOCK_FREQ;
// 1000ms = 1s
extern const uint64_t MSEC_PER_SEC;
// 时钟中断间隔
//extern static uint64_t INTERVAL;
// 触发时钟中断计数 < cycle
extern uint64_t TICKS;     
// 每秒触发时钟中断次数?   对于QEMU，时钟频率10MHz，每过1s，mtime增加10 000 000，也就是100ns，如果设置时间间隔100 000，每秒将发生100次中断
extern const uint64_t TICKS_PER_SEC;



uint64_t get_time_ms();

void timer_init();
void set_next_trigger();
void timer_handle();

uint64_t get_cycle();

/*
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define get_cycle() read_csr(cycle)
*/

TimeVal tm2tamp(struct tm *tm, int utc_diff);
struct tm *sys_time_get();
int sys_time_init(int year, int month, int day, int hour, int minute, int second);


#endif