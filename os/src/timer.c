#include "include/timer.h"
#include "include/riscv_asm.h"
#include "include/sbi.h"


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
    csr_set(CSR_SIE, IE_SIE);
    set_next_trigger();
}


void set_next_trigger(void)
{
    set_timer(get_time() + INTERVAL);
}