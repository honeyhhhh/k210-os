#include "include/mm.h"



static uint8_t KERNEL_HEAP_SPACE[KERNEL_HEAP_SIZE] = {0}; //bss

struct Heap_Allocator HEAP_ALLOCATOR;

