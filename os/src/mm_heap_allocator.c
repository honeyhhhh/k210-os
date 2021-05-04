#include "include/mm.h"



struct bitmap_buddy *HEAP_ALLOCATOR;
static uint8_t KERNEL_HEAP_ALLOCATOR[4096*4];
static uint8_t KERNEL_HEAP_SPACE[KERNEL_HEAP_SIZE] = {0}; //bss

void heap_test();

void heap_allocator_init()
{
    HEAP_ALLOCATOR = buddy_new(KERNEL_HEAP_SIZE, KERNEL_HEAP_ALLOCATOR);
    HEAP_ALLOCATOR->space = (uintptr_t *)KERNEL_HEAP_SPACE;
    printf("alloc space base: [%p] ", HEAP_ALLOCATOR->space);
    printf("memory remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    //heap_test();
}

void heap_test()
{
    // int *p0 = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    // printf("p0 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    // uint16_t *p1 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 16);
    // printf("p1 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    // int *p2 = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    // printf("p2 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    // uint16_t *p3 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 4);
    // printf("p3 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // uint64_t *p4 = (uint64_t *)buddy_alloc(HEAP_ALLOCATOR, 64);
    // printf("p4 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // uint64_t *p5 = (uint64_t *)buddy_alloc(HEAP_ALLOCATOR, 128);
    // printf("p5 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // int *p6 = (int *)buddy_alloc(HEAP_ALLOCATOR, sizeof(int)*100);
    // printf("p6 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // buddy_free(HEAP_ALLOCATOR, p5);
    // printf("p5 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // buddy_free(HEAP_ALLOCATOR, p1);
    // printf("p1 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    // uint16_t *p11 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 16);
    // printf("p11 remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    // printf("p11: [%p] \n", p11);

    // uint16_t *pmax = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 128*4096);
    // printf("pmax remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));

    int *a = buddy_alloc(HEAP_ALLOCATOR, sizeof(int)*10);
    for (int i = 0; i < 10; i++)
    {
        a[i] = i;
    }
    for (int i = 0; i < 10; i++)
    {
        printf("[%p]:[%d]\n", &a[i],a[i]);
    }
    printf("\n");
    
    

}

