#ifndef _BITMAP_H
#define _BITMAP_H

#include "stddef.h"

#define BITMAP_MASK 1

struct bitmap
{
    uint32_t btmp_bytes_len;
    uint8_t* bits;   //在遍历位图时，整体上以字节为单位，细节上是以位为单位。所以位图的指针必须是单字节
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);



#endif