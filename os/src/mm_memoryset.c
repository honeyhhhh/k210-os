#include "include/mm.h"
#include "include/riscv_asm.h"
#include "include/exception.h"


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
extern uintptr_t boot_stack_top;

/*      --- MapArea ---  */
struct MapArea *ma_new(VirtAddr start_va, VirtAddr end_va, MapType map_type, MapPermission map_perm)
{
    struct MapArea *self = (struct MapArea *)buddy_alloc(HEAP_ALLOCATOR ,sizeof(struct MapArea));

    self->vm_start = va_floor(start_va);
    self->vm_end = va_ceil(end_va);
    self->map_type = map_type;
    self->map_perm = map_perm;
    // 初始化 data_frames
    self->dataframe = NULL;

    return self;
}

void ma_map_one(struct MapArea *self, struct PageTable *page_table, VirtPageNum vpn)
{
    PhysPageNum ppn = 0;
    if (self->map_type == Identical)
    {
        ppn = (PhysPageNum)vpn;
    }
    if (self->map_type == Framed)
    {
        PhysPageNum frame = frame_alloc(&FRAME_ALLOCATOR);
        uint8_t *p = get_byte_array(frame);
        memset(p, 0, PAGE_SIZE);
        ppn = frame;
        // data_frames 插入（vpn 和 对应的 frame）
    }
    //printf("prepare : [%p]->[%p] \n", vpn, ppn);
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

void ms_init(struct MemorySet *self)
{
    pt_new(&self->page_table);
    self->head = NULL;
    self->tail = NULL;
}
void ms_free(struct MemorySet *self)
{
    // 调用pt_free 释放所有多级页表的节点所在的物理页帧
    // 调用ma_free 释放所有逻辑段中的数据所在的物理页帧(unmap?)
    // 这两部分 合在一起构成了一个地址空间所需的所有物理页帧。
}

uint64_t ms_token(struct MemorySet *self)
{
    return token(&self->page_table);
}


void ms_remove(struct MemorySet *self, VirtPageNum vpn_start)
{
    // 通过vpn在area中找到对应结点
    // unmap 的同时就会移除对应数据物理页帧
    // 然后在area链表中删除该节点
}

// 如果在当前地址空间插入一个 Framed 方式映射到 物理内存的逻辑段，要保证同一地址空间内的任意两个逻辑段不能存在交集
static void ms_push(struct MemorySet *self, struct MapArea *map_area, uint8_t *data, uint32_t size)
{
    // map
    ma_map(map_area, &self->page_table);
    // 拷贝数据
    if (data != NULL)  // 这里假设数据不超过一个页
    {
        // 切片 data 中的数据拷贝到当前逻辑段实际被内核放置在的各物理页帧上，从而 在地址空间中通过该逻辑段就能访问这些数据。
        //调用它的时候需要满足：切片 data 中的数据大小不超过当前逻辑段的 总大小，且切片中的数据会被对齐到逻辑段的开头，然后逐页拷贝到实际的物理页帧。
        PhysAddr dst = pte_get_pa(*(find_pte(&self->page_table, map_area->vm_start)));
        memmove((void *)dst, data, size);
        //printf("dst:[%p]", dst);
        //printf("    src:[%p]\n", data);
    }
    // 在area链表中插入新节点
    if (self->head == NULL)
    {
        self->head = map_area;
        self->tail = self->head;
        self->tail->next = self->head;
    }
    else
    {
        map_area->next = self->tail->next;
        self->tail->next = map_area;
        self->tail = map_area;
    }
}

void insert_framed_area(struct MemorySet *self, VirtAddr start_va, VirtAddr end_va, MapPermission perm)
{
    ms_push(self, ma_new(start_va, end_va, Framed, perm), NULL, 0);
}

void ms_map_trampoline(struct MemorySet *self)
{
    // 并没有新增逻辑段 MemoryArea 而是直接在多级页表中插入一个从地址空间的最高虚拟页面映射到 跳板汇编代码所在的物理页帧的键值对，访问方式限制与代码段相同，即 RX 。
    map(&self->page_table, va2vpn(TRAMPOLINE), pa2ppn((PhysAddr)strampoline), PTE_R | PTE_X);
}

void ms_map_kcontext(struct MemorySet *self)
{
    PhysPageNum f = frame_alloc(&FRAME_ALLOCATOR);
    //printf("%p\n", kernelcon);
    kernelcon = (struct context *)ppn2pa(f);
    //printf("%p\n", &kernelcon);
    kernelcon->kernel_satp = token(&self->page_table);
    //printf("%p\n", &kernelcon);
    kernelcon->kernel_sp = (uintptr_t)&boot_stack_top;
    kernelcon->trap_handle = (uintptr_t)&e_dispatch;
    map(&self->page_table, va2vpn(TRAP_CONTEXT), f, PTE_R|PTE_W);
}


// static const struct MemmapEntry {
//     uint64_t base;
//     uint64_t size;
// } virt_memmap[] = {
//     [VIRT_DEBUG] =       {        0x0,         0x100 },
//     [VIRT_MROM] =        {     0x1000,        0xf000 },
//     [VIRT_TEST] =        {   0x100000,        0x1000 },
//     [VIRT_RTC] =         {   0x101000,        0x1000 },
//     [VIRT_CLINT] =       {  0x2000000,       0x10000 },
//     [VIRT_PCIE_PIO] =    {  0x3000000,       0x10000 },
//     [VIRT_PLIC] =        {  0xc000000, VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2) },
//     [VIRT_UART0] =       { 0x10000000,         0x100 },
//     [VIRT_VIRTIO] =      { 0x10001000,        0x1000 },
//     [VIRT_FLASH] =       { 0x20000000,     0x4000000 },
//     [VIRT_PCIE_ECAM] =   { 0x30000000,    0x10000000 },
//     [VIRT_PCIE_MMIO] =   { 0x40000000,    0x40000000 },
//     [VIRT_DRAM] =        { 0x80000000,           0x0 },
// };
static const struct MemmapEntry {
    uint64_t base;
    uint64_t size;
} k210_memmap[] = {
    [K210_CLINT] =       { 0x02000000,       0x10000 },
    [QEMU_PLIC] =        { 0x0C000000,        0x3000 },	
    [K210_PLIC] =        { 0x0C200000,        0x1000 },
    [QEMU_UART0] =       { 0x10000000,        0x1000 },
    [QEMU_VIRTIO0] =     { 0x10001000,        0x1000 },
    [K210_UARTHS] =      { 0x38000000,        0x1000 },
    [K210_GPIOHS] =      { 0x38001000,        0x1000 },
    [K210_GPIO] =        { 0x50200000,        0x1000 },
    [K210_SPI_SLAVE] =   { 0x50240000,        0x1000 },
    [K210_FPIOA] =       { 0x502B0000,        0x1000 },
    [K210_TIMER0] =      { 0x502D0000,        0x1000 },
    [K210_TIMER1] =      { 0x502E0000,        0x1000 },
    [K210_TIMER2] =      { 0x502F0000,        0x1000 },
    [K210_SYSCTL] =      { 0x50440000,        0x1000 },
    [K210_SPI0] =        { 0x52000000,        0x1000 },
    [K210_SPI1] =        { 0x53000000,        0x1000 },
    [K210_SPI2] =        { 0x54000000,        0x1000 },
    [K210_DMAC] =        { 0x50000000,        0x1000 },
};

void new_kenel(struct MemorySet *KERNEL_SPACE)
{
    ms_init(KERNEL_SPACE);
    
    // 无新增逻辑段
    ms_map_trampoline(KERNEL_SPACE); // 最高页面 
    ms_map_kcontext(KERNEL_SPACE);  //　次高页面 





    // 注意 权限
    printf("mapping rustsbi\n"); // 没必要
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)RUSTSBI_BASE, (VirtAddr)skernel, Identical, VM_R|VM_X), NULL, 0);

    printf("mapping .text\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)stext, (VirtAddr)etext, Identical, VM_R|VM_X), NULL, 0);
    printf("mapping .rodata\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)srodata, (VirtAddr)erodata, Identical, VM_R), NULL, 0);
    printf("mapping .data\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)sdata, (VirtAddr)edata, Identical, VM_R|VM_W), NULL, 0);
    printf("mapping .bss\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)sbss_with_stack, (VirtAddr)ebss, Identical, VM_R|VM_W), NULL, 0);
    printf("mapping ekernel~MEMORY_END\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)(&ekernel), (VirtAddr)MEMORY_END, Identical, VM_R|VM_W), NULL, 0);


    printf("mapping memory-mapped registers\n");
    for (int i = 0; i < sizeof(k210_memmap)/sizeof(k210_memmap[0]); i++)
    {
        if (i != 0 && i != 1 && i != 4 && i != 3)
            ms_push(KERNEL_SPACE, ma_new((VirtAddr)k210_memmap[i].base, (VirtAddr)(k210_memmap[i].base+k210_memmap[i].size), Identical, VM_R|VM_W), NULL, 0);
        //printf("%d\n", i);
        //printf("%d\n", buddy_remain_size(HEAP_ALLOCATOR));
    }
}



void page_activate()
{
    new_kenel(&KERNEL_SPACE);
    printf("kenel map done !\n");
    page_init();
    printf("remain frame: [%d]\n", frame_remain_size(&FRAME_ALLOCATOR));
    printf("remain kheap size: [%d]\n", buddy_remain_size(HEAP_ALLOCATOR));
    // 这条写入 satp 的指令及其下一条指令都在内核内存布局的代码段中，在切换之后是一个恒等映射， 而在切换之前是视为物理地址直接取指，也可以将其看成一个恒等映射。这完全符合我们的期待：即使切换了地址空间，指令仍应该 能够被连续的执行。
}

void page_init()
{
    uint64_t satp = token(&KERNEL_SPACE.page_table);
    csr_write(CSR_SATP, satp);
    //asm volatile("csrw satp, %0" : : "r" (satp));
    printf("satp [%p]\n", satp);
    printf("paging ... done!\n");
    //flush_tlb_all();
    do {
        asm volatile("fence");
        asm volatile("fence.i");
        asm volatile("sfence.vma" : : : "memory");
        asm volatile("fence");
        asm volatile("fence.i");
    } while(0);
    printf("flush\n");
}



void ms_map_uinit(struct MemorySet *ums, uint8_t *initcode, uint32_t icsize, struct context *trap_cx, uint32_t cxsize)
{
    ms_init(ums);

    // Ttampoline
    ms_map_trampoline(ums);

    // Trap Context
    ms_push(ums, ma_new((VirtAddr)TRAP_CONTEXT, (VirtAddr)TRAMPOLINE, Framed, VM_R|VM_W), (void *)trap_cx, cxsize);  //物理页号通过find_pte获取

    // initcode   [s, e)
    struct MapArea *ma =  ma_new((VirtAddr)0, (VirtAddr)icsize, Framed, VM_R|VM_W|VM_X|VM_U);
    ms_push(ums, ma, initcode, icsize);

    // userstack
    VirtAddr user_stack_bottom = vpn2va(va_ceil((VirtAddr)icsize + PAGE_SIZE));
    VirtAddr user_stack_top = user_stack_bottom + USER_STACK_SIZE;
    ms_push(ums, ma_new(user_stack_bottom, user_stack_top, Framed, VM_R|VM_W|VM_U), NULL, 0);

}