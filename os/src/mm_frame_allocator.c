#include "include/mm.h"
#include "include/max_heap.h"

struct FrameAllocator FRAME_ALLOCATOR;


static void _frame_allocator_init(PhysPageNum l, PhysPageNum r)
{
    FRAME_ALLOCATOR.current = l;
    FRAME_ALLOCATOR.end = r;
    printf("remain %d (%p ~ %p) Phys Frames \n", FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current, l , r);
}

void frame_allocator_init()
{
    _frame_allocator_init(pa_ceil((PhysAddr)&ekernel), pa_floor((PhysAddr)MEMORY_END));
}