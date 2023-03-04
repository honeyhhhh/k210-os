#ifndef _BUDDY_H
#define _BUDDY_H

#include "stddef.h"
#include "spinlock.h"

// 假设管理的内存单元个数为n（字节）
// 则二叉树结点为2n-1
// 如果最小分配单元为2字节，则二叉树结点为n-1
// 如果最小分配单元为4字节，则二叉树结点为n/2-1
// 如果最小分配单元为8字节，则二叉树结点为n/4-1
// 此时每个结点占用1位空间，则所有结点占用 n/32 字节 - 1位
// 但是很难通过01计算得到结点真实值

// 将最后两层 即8、16字节层使用bitmap
// 这样将占用 (n/16-1)字节 + (n/8 + n/16)/8字节  ≈  n/10




// 先设置 内核临时堆128KB ，该分配器需要占用11KB+
struct bitmap_buddy
{
    struct spinlock lock;
    uintptr_t *space;
    uint64_t size;
    uint64_t min_alloc_size;
    uint8_t bbt[1];
};

struct bitmap_buddy* buddy_new(int size, void *bbase);
//void buddy_destroy(struct bitmap_buddy *b);

void *buddy_alloc(struct bitmap_buddy* b, uint32_t size);
void buddy_free(struct bitmap_buddy* b, void *ptr);

uint64_t buddy_remain_size(struct bitmap_buddy *b);
int buddy_size(struct bitmap_buddy *b, uint64_t offset);




#endif