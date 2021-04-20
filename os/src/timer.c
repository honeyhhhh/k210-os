#include "include/timer.h"
#include "include/riscv_asm.h"
#include "include/sbi.h"
#include "include/stdio.h"

// QEMU_CLOCK_FREQ
const uint64_t QEMU_CLOCK_FREQ = 10000000;
// 1000ms = 1s
const uint64_t MSEC_PER_SEC = 1000;
// 时钟中断间隔
static uint64_t INTERVAL = 100000;
// 触发时钟中断计数 < cycle
uint64_t TICKS = 0;      
// 每秒触发时钟中断次数?   对于QEMU，时钟频率10MHz，每过1s，mtime增加10 000 000，也就是100ns，如果设置时间间隔100 000，每秒将发生100次中断
const uint64_t TICKS_PER_SEC = 100;

static inline uint64_t get_time(void)
{
    return csr_read(CSR_TIME);
}

uint64_t get_time_ms(void)
{
    return get_time() / (QEMU_CLOCK_FREQ / MSEC_PER_SEC);
}


static inline uint64_t get_time_v2(void)
{
#if __riscv_xlen == 64
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
#else
    uint32_t lo, hi, tmp;
    __asm__ __volatile__(
        "1:\n"
        "rdtimeh %0\n"
        "rdtime %1\n"
        "rdtimeh %2\n"
        "bne %0, %2, 1b"
        : "=&r"(hi), "=&r"(lo), "=&r"(tmp)
    );
    return ((uint64_t)hi << 32) | lo;
#endif
}

static inline uint64_t get_cycle(void)
{
    return csr_read(CSR_CYCLE);
}


// SIE.STIE = 1 并 set_timer
void timer_init(void)
{
    csr_set(CSR_SIE, IE_TIE);
    set_next_trigger();
    printf("timer start work....\n");
    printf("%d\n%d\n%d\n", get_time(), get_time_ms(), get_time_v2());
}


void set_next_trigger(void)
{
    set_timer(get_time() + INTERVAL);
}