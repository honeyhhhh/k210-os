#include "include/stddef.h"
#include "include/sysctl.h"
#include "include/timer.h"



int usleep(uint64_t usec)
{
    uint64_t cycle = get_cycle();

    uint64_t nop_all = usec * sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL;
    while(1)
    {
        if(get_cycle() - cycle >= nop_all)
            break;
    }
    return 0;
}

int msleep(uint64_t msec)
{
    return (unsigned int)usleep(msec * 1000);
}

unsigned int ksleep(unsigned int seconds)
{
    return (unsigned int)msleep(seconds * 1000);
}