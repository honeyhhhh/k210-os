#include "include/clint.h"
#include "include/sysctl.h"

volatile clint_t *const clint = (volatile clint_t *)CLINT_BASE_ADDR;

uint64_t clint_get_time(void)
{
    /* No difference on cores */
    return clint->mtime;
}

uint64_t clint_timer_get_freq(void)
{
    /* The clock is divided by CLINT_CLOCK_DIV */
    return sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / CLINT_CLOCK_DIV;
}
