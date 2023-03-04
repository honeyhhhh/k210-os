#include "include/mm.h"
#include "include/max_heap.h"

struct FrameAllocator FRAME_ALLOCATOR;


void frame_test();
void frame_allocator_init()
{
    //printk("frame alloc init\n");
    FRAME_ALLOCATOR.current = pa_ceil((PhysAddr)&ekernel + PAGE_SIZE);
    FRAME_ALLOCATOR.end = pa_floor((PhysAddr)MEMORY_END);
    uint64_t page_nums = FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current;
    printk("remain %d (%p ~ %p) Phys Frames \n", page_nums, FRAME_ALLOCATOR.current, FRAME_ALLOCATOR.end);

    // printk("[%p]\n", HEAP_ALLOCATOR);
    heap_init(&FRAME_ALLOCATOR.recycled, NULL , page_nums);

    //printk("%p\n", FRAME_ALLOCATOR.recycled.heapArray);
    // frame_test();
}

PhysPageNum frame_alloc(struct FrameAllocator *self)
{
    PhysPageNum re = 0;
    if (!heap_empty(&self->recycled))
    {
        //printk("frame alloc from maxheap!\n");
        // PhysPageNum t = (PhysPageNum)heap_top(&self->recycled);
        // heap_removeMax(&self->recycled);
        PhysPageNum t = (PhysPageNum)self->recycled.heapArray[self->recycled.CurrentSize-1];
        self->recycled.CurrentSize--;      
        re = t;
    }
    else
    {
        if (self->current == self->end)
        {
            panic("no frame to alloc !\n");
        }
        else
        {
            //printk("frame alloc from current!\n");

            self->current++;
            re = self->current - 1;
        }
    }
    return re;
}

void frame_dealloc(struct FrameAllocator *self, PhysPageNum ppn)
{
    //printk("free page :[%p]\n", ppn);
    if ((uint64_t)ppn >= self->current || is_some(&self->recycled, (uint64_t)ppn))
        panic("Frame ppn={%p} has not been allocated!\n", ppn);
    heap_insert(&self->recycled, (uint64_t)ppn);

    while (heap_top(&self->recycled) == (uint64_t)self->current - 1)
    {
        heap_removeMax(&self->recycled);
        self->current--;
    }

}

uint64_t frame_remain_size(struct FrameAllocator *self)
{
    return self->end - self->current + self->recycled.CurrentSize;
}

void frame_test()
{
    PhysPageNum p1 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p1 :[%p]\n", p1);
    PhysPageNum p2 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p2 :[%p]\n", p2);
    PhysPageNum p3 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p3 :[%p]\n", p3);
    PhysPageNum p4 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p4 :[%p]\n", p4);
    PhysPageNum p5 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p5 :[%p]\n", p5);
    PhysPageNum p6 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page p6 :[%p]\n", p6);

    frame_dealloc(&FRAME_ALLOCATOR, p4);    
    frame_dealloc(&FRAME_ALLOCATOR, p3);    
    // frame_dealloc(&FRAME_ALLOCATOR, p6);    
    frame_dealloc(&FRAME_ALLOCATOR, p5);    

    PhysPageNum p7 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page :[%p]\n", p7);
    PhysPageNum p8 = frame_alloc(&FRAME_ALLOCATOR);
    printk("alloc page :[%p]\n", p8);

    // frame_dealloc(&FRAME_ALLOCATOR, p4);    


}