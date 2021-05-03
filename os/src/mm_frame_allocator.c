#include "include/mm.h"
#include "include/max_heap.h"

struct FrameAllocator FRAME_ALLOCATOR;



void frame_allocator_init()
{
    FRAME_ALLOCATOR.current = pa_ceil((PhysAddr)&ekernel);
    FRAME_ALLOCATOR.end = pa_floor((PhysAddr)MEMORY_END);
    uint64_t page_nums = FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current;
    printf("remain %d (%p ~ %p) Phys Frames \n", page_nums, FRAME_ALLOCATOR.current, FRAME_ALLOCATOR.end);


    //heap_init(&FRAME_ALLOCATOR.recycled, , total_page_num);
}