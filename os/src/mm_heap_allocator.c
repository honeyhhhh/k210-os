#include "include/mm.h"



struct bitmap_buddy *HEAP_ALLOCATOR;
static uint8_t KERNEL_HEAP_ALLOCATOR[4096*2];
static uint8_t KERNEL_HEAP_SPACE[KERNEL_HEAP_SIZE] = {0}; //bss


void heap_allocator_init()
{
    HEAP_ALLOCATOR = buddy_new(KERNEL_HEAP_SIZE, KERNEL_HEAP_ALLOCATOR);
    HEAP_ALLOCATOR->space = (uintptr_t *)KERNEL_HEAP_SPACE;
}

