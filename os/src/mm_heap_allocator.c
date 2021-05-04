#include "include/mm.h"



struct bitmap_buddy *HEAP_ALLOCATOR;
static uint8_t KERNEL_HEAP_ALLOCATOR[4096*4];
static uint8_t KERNEL_HEAP_SPACE[KERNEL_HEAP_SIZE] = {0}; //bss


void heap_allocator_init()
{
    HEAP_ALLOCATOR = buddy_new(KERNEL_HEAP_SIZE, KERNEL_HEAP_ALLOCATOR);
    HEAP_ALLOCATOR->space = (uintptr_t *)KERNEL_HEAP_SPACE;
    printf("remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    int *p = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    printf("remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    //uint16_t *p1 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 16);
    //printf("remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    //int *p2 = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    //printf("remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    
}

