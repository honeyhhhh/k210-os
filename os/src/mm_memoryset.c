#include "include/mm.h"
#include "include/riscv_asm.h"


struct MemorySet KERNEL_SPACE;

extern uintptr_t strampoline[];
extern uintptr_t stext[];
extern uintptr_t etext[];
extern uintptr_t srodata[];
extern uintptr_t erodata[];
extern uintptr_t sdata[];
extern uintptr_t edata[];
extern uintptr_t sbss_with_stack[];
extern uintptr_t ebss[];
extern uintptr_t ekernel[];

/*      --- MapArea ---  */
struct MapArea *ma_new(VirtAddr start_va, VirtAddr end_va, MapType map_type, MapPermission map_perm)
{
    struct MapArea *self = (struct MapArea *)buddy_alloc(HEAP_ALLOCATOR ,sizeof(struct MapArea));

    self->vm_start = va_floor(start_va);
    self->vm_end = va_ceil(end_va);
    self->map_type = map_type;
    self->map_perm = map_perm;
    // 初始化 data_frames

    return self;
}

void ma_map_one(struct MapArea *self, struct PageTable *page_table, VirtPageNum vpn)
{
    PhysPageNum ppn;
    if (self->map_type == Identical)
    {
        ppn = (PhysPageNum)vpn;
    }
    if (self->map_type == Framed)
    {
        PhysPageNum frame = frame_alloc(&FRAME_ALLOCATOR);
        ppn = frame;
        // data_frames 插入（vpn 和 对应的 frame）
    }
    map(page_table, vpn, ppn, (PTEFlags)self->map_perm);
}

void ma_unmap_one(struct MapArea *self, struct PageTable *page_table, VirtPageNum vpn)
{
    if (self->map_type == Framed)
    {
        // data_frames 删除(vpn ) ?  立即释放对应frame以便后续分配
    }
    unmap(page_table,vpn);
}

void ma_map(struct MapArea *self, struct PageTable *page_table)
{
    for (VirtPageNum vpn = self->vm_start; vpn < self->vm_end ; vpn++)
    {
        ma_map_one(self, page_table, vpn);
    }
    
}
void ma_unmap(struct MapArea *self, struct PageTable *page_table)
{
    for (VirtPageNum vpn = self->vm_start; vpn < self->vm_end ; vpn++)
    {
        ma_unmap_one(self, page_table, vpn);
    }
    
}



/*      --- MemorySet ---  */

void ms_new(struct MemorySet *self)
{
    pt_new(&self->page_table);

}
void ms_free(struct MemorySet *self)
{

}

uint64_t ms_token(struct MemorySet *self)
{
    return token(&self->page_table);
}

void ms_insert_framed_area(struct MemorySet *self, VirtAddr va_start, VirtAddr va_end, MapPermission perm)
{

}

void ms_remove_area_with_vstart(struct MemorySet *self, VirtPageNum vpn_start)
{

}

static void ms_push(struct MemorySet *self, struct MapArea *map_area)
{
    ma_map(self, &self->page_table);
    // push map_area
}

void ms_map_trampoline(struct MemorySet *self)
{
    map(&self->page_table, va2vpn(TRAMPOLINE), pa2ppn(strampoline), PTE_R | PTE_X);
}

void new_kenel()
{
    ms_new(&KERNEL_SPACE);
    ms_map_trampoline(&KERNEL_SPACE);

    printf("mapping .text\n");
    ms_push(&KERNEL_SPACE, ma_new((VirtAddr)stext, (VirtAddr)etext, Identical, VM_R|VM_X));
    printf("mapping .rodata\n");
    ms_push(&KERNEL_SPACE, ma_new((VirtAddr)srodata, (VirtAddr)erodata, Identical, VM_R));
    printf("mapping .data\n");
    ms_push(&KERNEL_SPACE, ma_new((VirtAddr)sdata, (VirtAddr)edata, Identical, VM_R|VM_X));
    printf("mapping .bss\n");
    ms_push(&KERNEL_SPACE, ma_new((VirtAddr)sbss_with_stack, (VirtAddr)ebss, Identical, VM_R|VM_X));
    printf("mapping ekernel~MEMORY_END\n");
    ms_push(&KERNEL_SPACE, ma_new((VirtAddr)ekernel, (VirtAddr)MEMORY_END, Identical, VM_R|VM_X));


    printf("mapping memory-mapped registers\n");
    ms_push(&KERNEL_SPACE, )
}



void page_activate()
{
    uint64_t satp = token(&KERNEL_SPACE.page_table);
    csr_write(CSR_SATP, satp);
    flush_tlb_all();
    // 这条写入 satp 的指令及其下一条指令都在内核内存布局的代码段中，在切换之后是一个恒等映射， 而在切换之前是视为物理地址直接取指，也可以将其看成一个恒等映射。这完全符合我们的期待：即使切换了地址空间，指令仍应该 能够被连续的执行。
}

