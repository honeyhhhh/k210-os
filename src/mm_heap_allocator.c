#include "include/mm.h"


//extern struct bitmap_buddy *HEAP_ALLOCATOR[];
//struct bitmap_buddy *HEAP_ALLOCATOR = (struct bitmap_buddy*)erodata;
static uint8_t KERNEL_HEAP_ALLOCATOR[4096*128];
static uint8_t KERNEL_HEAP_SPACE[KERNEL_HEAP_SIZE] = {0}; //bss
// static uint8_t KERNEL_HEAP_ALLOCATOR2[4096*64];
// static uint8_t KERNEL_HEAP_SPACE2[KERNEL_HEAP_SIZE] = {0}; //bss

void heap_test();

void heap_allocator_init()
{
    //printk("heap alloc init\n");
    HEAP_ALLOCATOR = buddy_new(KERNEL_HEAP_SIZE, KERNEL_HEAP_ALLOCATOR);
    HEAP_ALLOCATOR->space = (uintptr_t *)KERNEL_HEAP_SPACE;

    // HEAP_ALLOCATOR2 = buddy_new(KERNEL_HEAP_SIZE, KERNEL_HEAP_ALLOCATOR2);
    // HEAP_ALLOCATOR2->space = (uintptr_t *)KERNEL_HEAP_SPACE2;

    //printk("alloc space base: [%p] ", &HEAP_ALLOCATOR->space);
    printk("memory remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    //heap_test();

    //printk("[%p]\n",HEAP_ALLOCATOR);
}

void heap_test()
{
    int *p0 = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    buddy_free(HEAP_ALLOCATOR, p0);
    uint16_t *p1 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 16);
    int *p2 = (int *)buddy_alloc(HEAP_ALLOCATOR, 4);
    uint16_t *p3 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 4);

    uint64_t *p4 = (uint64_t *)buddy_alloc(HEAP_ALLOCATOR, 64);

    uint64_t *p5 = (uint64_t *)buddy_alloc(HEAP_ALLOCATOR, 128);

    int *p6 = (int *)buddy_alloc(HEAP_ALLOCATOR, sizeof(int)*100);

    uint16_t *p11 = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 16);
    buddy_free(HEAP_ALLOCATOR, p1);

    uint16_t *pmax = (uint16_t *)buddy_alloc(HEAP_ALLOCATOR, 128*4096);
    buddy_free(HEAP_ALLOCATOR, p2);
    buddy_free(HEAP_ALLOCATOR, p3);
    buddy_free(HEAP_ALLOCATOR, p4);
        buddy_free(HEAP_ALLOCATOR, p0);
    buddy_free(HEAP_ALLOCATOR, p0);

    buddy_free(HEAP_ALLOCATOR, p11);
        buddy_free(HEAP_ALLOCATOR, p5);
    buddy_free(HEAP_ALLOCATOR, p6);
        buddy_free(HEAP_ALLOCATOR, pmax);




    // int *a = buddy_alloc(HEAP_ALLOCATOR, sizeof(int)*10);
    // for (int i = 0; i < 10; i++)
    // {
    //     a[i] = i;
    // }
    // for (int i = 0; i < 10; i++)
    // {
    //     printk("[%p]:[%d]\n", &a[i],a[i]);
    // }
    // printk("\n");
    panic("..");
    

}

