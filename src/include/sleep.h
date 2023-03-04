#ifndef _BSP_SLEEP_H
#define _BSP_SLEEP_H

#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

int usleep(uint64_t usec);
int msleep(uint64_t msec);
unsigned int ksleep(unsigned int seconds);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_SLEEP_H */