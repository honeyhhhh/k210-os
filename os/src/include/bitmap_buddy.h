#ifndef _BUDDY_H
#define _BUDDY_H

#include "stddef.h"

// 假设管理的内存单元个数为n（字节）
// 则二叉树结点为2n
// 如果最小分配单元为2字节，则二叉树结点为n
// 如果最小分配单元为4字节，则二叉树结点为n/2
// 如果最小分配单元为8字节，则二叉树结点为n/4
// 此时每个结点占用1位空间，则所有结点占用 n/32 字节

// 先设置 内核临时堆128KB ，该分配器需要占用4KB空间+
struct bitmap_buddy
{
    uintptr_t *space;
    uint64_t size;
    uint8_t bbt[1];
};

struct bitmap_buddy* buddy_new(int size, void *bbase);
void buddy_destroy(struct bitmap_buddy *b);

int buddy_alloc(struct bitmap_buddy* b, int size);
void buddy_free(struct bitmap_buddy* b, int offset);

uint64_t buddy_remain_size(struct bitmap_buddy *b);
int buddy_size(struct bitmap_buddy *b, int offset);
void buddy_dump(struct bitmap_buddy *b);




#endif