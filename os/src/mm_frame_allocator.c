#include "include/mm.h"
#include "include/max_heap.h"

struct FrameAllocator FRAME_ALLOCATOR;


static void _frame_allocator_init(PhysPageNum l, PhysPageNum r)
{
    FRAME_ALLOCATOR.current = l;
    FRAME_ALLOCATOR.end = r;
    printf("remain %d Phys Frames \n", FRAME_ALLOCATOR.current - FRAME_ALLOCATOR.end);
}

void frame_allocator_init()
{
    _frame_allocator_init(pa2ppn(pa_ceil((PhysAddr)&ekernel)), pa2ppn(pa_floor((PhysAddr)MEMORY_END)));
}