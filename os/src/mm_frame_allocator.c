#include "include/mm.h"
#include "include/max_heap.h"

struct FrameAllocator FRAME_ALLOCATOR;



void frame_allocator_init()
{
    FRAME_ALLOCATOR.current = pa_ceil((PhysAddr)&ekernel);
    FRAME_ALLOCATOR.end = pa_floor((PhysAddr)MEMORY_END);
    uint64_t page_nums = FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current;
    printf("remain %d (%p ~ %p) Phys Frames \n", page_nums, FRAME_ALLOCATOR.current, FRAME_ALLOCATOR.end);


    heap_init(&FRAME_ALLOCATOR.recycled,  buddy_alloc(HEAP_ALLOCATOR, 8*page_nums), page_nums);

    printf("%p\n", FRAME_ALLOCATOR.recycled.heapArray);
}

PhysPageNum frame_alloc(struct FrameAllocator *self)
{
    if (!heap_empty(self->recycled.heapArray))
    {
        return (PhysPageNum)heap_top(self->recycled.heapArray);
    }
    else
    {
        if (self->current == self->end)
        {
            panic("no frame to alloc !\n");
        }
        else
        {
            self->current++;
            return self->current - 1;
        }
    }
}

void frame_dealloc(struct FrameAllocator *self, PhysPageNum ppn)
{
    if ((uint64_t)ppn >= self->current || is_some(self, (uint64_t)ppn))
        panic("Frame ppn={%p} has not been allocated!", ppn);
    heap_insert(self, (uint64_t)ppn);
}