#include "include/mm.h"
#include "include/riscv_asm.h"
#include "include/exception.h"
#include "include/elf.h"
#include "include/proc.h"


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
    self->dataframe = (struct vpmap *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct vpmap));
    self->dataframe_tail = self->dataframe;
    self->dataframe->next = self->dataframe_tail;

    return self;
}

void ma_free(struct MapArea *ma)
{
    // 只
}

void vpmap_push(struct vpmap **tail, struct vpmap *node)
{
    node->next = (*tail)->next;
    (*tail)->next = node;
    (*tail) = node;
}
struct vpmap *vpmap_remove(struct vpmap *head, VirtPageNum node)
{
    struct vpmap *pre = head;
    struct vpmap *p = head->next;
    while (p != head)
    {
       if (p->vpn == node)
       {
           pre->next = p->next;
           p->next = NULL;
           return p;
       }
       pre= p;
       p = p->next;
    }
    return NULL;
    
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

        struct vpmap *node = (struct vpmap *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct vpmap));
        node->ppn = ppn;
        node->vpn = vpn;
        vpmap_push(&self->dataframe_tail, node);
    }
    //printk("prepare : [%p]->[%p] \n", vpn, ppn);
    map(page_table, vpn, ppn, (PTEFlags)self->map_perm);
}

void ma_unmap_one(struct MapArea *self, struct PageTable *page_table, VirtPageNum vpn)
{
    if (self->map_type == Framed)
    {
        // data_frames 删除(vpn ) ?  立即释放对应frame以便后续分配
        struct vpmap *node = vpmap_remove(self->dataframe, vpn);
        if (node == NULL)
          panic("unknow fault!\n");
        frame_dealloc(&FRAME_ALLOCATOR, node->ppn);
        buddy_free(HEAP_ALLOCATOR, node);
    }
    unmap(page_table,vpn);
}

void ma_map(struct MapArea *self, struct PageTable *page_table)
{
    //printk("start :%d end :%d\n",self->vm_start, self->vm_end);
    for (VirtPageNum vpn = self->vm_start; vpn < self->vm_end ; vpn++)
    {
        //printk("map %d\n", vpn);
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

//可以从一个逻辑段复制得到一个虚拟地址区间、映射方式和权限控制均相同的逻辑段，不同的是由于它还没有真正被映射到物理页帧上
struct MapArea *from_another(struct MapArea *a)
{
    struct MapArea *self = (struct MapArea *)buddy_alloc(HEAP_ALLOCATOR ,sizeof(struct MapArea));

    self->vm_start = a->vm_start;
    self->vm_end = a->vm_end;
    self->map_type = a->map_type;
    self->map_perm = a->map_perm;
    // 初始化 data_frames
    self->dataframe = (struct vpmap *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct vpmap));
    self->dataframe_tail = self->dataframe;
    self->dataframe->next = self->dataframe_tail;

    return self;    
}




/*      --- MemorySet ---  */

void ms_init(struct MemorySet *self)
{
    pt_new(&self->page_table);

    list_init(&self->vma);
    list_init(&self->areas);
}
void ms_free(struct MemorySet *self)
{
    // 调用pt_free 释放所有多级页表的节点所在的物理页帧  destroy_fvec
    pt_free(&self->page_table);
    // 调用ms_ma_free 释放所有逻辑段中的数据所在的物理页帧(unmap?)
    ms_ma_free(self);
    // 这两部分 合在一起构成了一个地址空间所需的所有物理页帧。

}

void ms_ma_free(struct MemorySet *self)
{
    struct MapArea *p = NULL;
    while (!list_empty(&self->areas))
    {
        p = elem2entry(struct MapArea , tag, list_pop(&self->areas));
        ma_unmap(p, &self->page_table);
        buddy_free(HEAP_ALLOCATOR, p->dataframe);
        buddy_free(HEAP_ALLOCATOR, p);
    }

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
        if (size < PAGE_SIZE)
        {
            PhysAddr dst = pte_get_pa(*(find_pte(&self->page_table, map_area->vm_start)));
            memmove((void *)dst, data, PAGE_SIZE);
          //printk("dst:[%p]  <-- src [%p]  size = [%d]\n", dst, data,PAGE_SIZE);
        }
        else
        {
            VirtPageNum vpn;
            PhysAddr dst;
            uint64_t off;
            for (vpn = map_area->vm_start, off = 0; vpn < map_area->vm_end ; vpn++, off += PAGE_SIZE)
            {
                dst = pte_get_pa(*(find_pte(&self->page_table, vpn)));
                memmove((void *)(dst), data + off, PAGE_SIZE);
                //printk("dst:[%p]  <-- src [%p]  size = [%d]\n", dst, data + off, PAGE_SIZE);
            }

        }
    }
    // 在area链表中插入新节点
    list_append(&self->areas, &map_area->tag);
    // if (self->head == NULL)
    // {
    //     self->head = map_area;
    //     self->tail = self->head;
    //     self->tail->next = self->head;
    // }
    // else
    // {
    //     map_area->next = self->tail->next;
    //     self->tail->next = map_area;
    //     self->tail = map_area;
    // }

}


void insert_framed_area(struct MemorySet *self, VirtAddr start_va, VirtAddr end_va, MapPermission perm)
{
    ms_push(self, ma_new(start_va, end_va, Framed, perm), NULL, 0);
}



void brk(uint64_t n)
{
    struct TaskControlBlock *p = mytask();
    uint64_t newsize = p->base_size;

    if (n > 0)
    {
        if (va_ceil(newsize) == va_ceil(newsize + n))
        {
            p->base_size = newsize + n;

            return ;
        }

        // 保证　与原来的地址空间　不存在交集
        ms_push(p->memory_set, ma_new((VirtAddr)newsize, (VirtAddr)(newsize + n), Framed, VM_R|VM_U|VM_X|VM_W), NULL, 0);
 
    }
    else if (n < 0)
    {
        // 判断　floor(newsize - n)   和　floor(　newsize )  决定是否remove
    }

    p->base_size = newsize + n;

}






//可以复制一个完全相同的地址空间
//剩下的逻辑段都包含在 areas 中。我们遍历原地址空间中的所有逻辑段，将复制之后的逻辑段插入新的地址空间，在插入的时候就已经实际分配了物理页帧了。
//接着我们遍历逻辑段中的每个虚拟页面，对应完成数据复制，这只需要找出两个地址空间中的虚拟页面各被映射到哪个物理页帧，就可转化为将数据从物理内存中的一个位置复制到另一个位置
struct MemorySet *from_existed_user(struct MemorySet *a)
{

    struct MemorySet *ms = (struct MemorySet *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct MemorySet));

    ms_init(ms);
    struct MapArea *ma = NULL;
    struct MapArea *newma = NULL;
    
    ms_map_trampoline(ms);
    
    //acquire(&a->areas.lock);
    struct list_elem* elem = a->areas.head.next;

    while(elem != &a->areas.tail) 
    {

        ma = elem2entry(struct MapArea, tag, elem);
        newma = from_another(ma);
        ms_push(ms, newma, NULL, 0);

        for (VirtPageNum vpn = ma->vm_start; vpn < ma->vm_end; vpn++)
        {

            // 将 父　虚拟页　p　的内容　移动到　子　虚拟页　p
            PhysAddr srcpa = pte_get_pa(*(find_pte(&a->page_table, vpn)));
            PhysAddr dstpa = pte_get_pa(*(find_pte(&ms->page_table, vpn)));

            memmove((void *)dstpa, (void *)srcpa, PAGE_SIZE);
        }



        elem = elem->next;
    }
    //release(&a->areas.lock);



    return ms;
}




void ms_map_trampoline(struct MemorySet *self)
{
    // 并没有新增逻辑段 MemoryArea 而是直接在多级页表中插入一个从地址空间的最高虚拟页面映射到 跳板汇编代码所在的物理页帧的键值对，访问方式限制与代码段相同，即 RX 。
    map(&self->page_table, va2vpn(TRAMPOLINE), pa2ppn((PhysAddr)strampoline), PTE_R | PTE_X);
}

void ms_map_kcontext(struct MemorySet *self)
{
    PhysPageNum f = frame_alloc(&FRAME_ALLOCATOR);
    //printk("%p\n", kernelcon);
    kernelcon = (struct context *)ppn2pa(f);
    //printk("%p\n", &kernelcon);
    kernelcon->kernel_satp = token(&self->page_table);
    //printk("%p\n", &kernelcon);
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
    [K210_RTC] =         { 0x50460000,        0x1000 },
};

void new_kenel(struct MemorySet *KERNEL_SPACE)
{
    ms_init(KERNEL_SPACE);
    
    // 无新增逻辑段
    ms_map_trampoline(KERNEL_SPACE); // 最高页面 
    ms_map_kcontext(KERNEL_SPACE);  //　次高页面 





    // 注意 权限
  //printk("mapping rustsbi\n"); // 没必要
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)RUSTSBI_BASE, (VirtAddr)skernel, Identical, VM_R|VM_X), NULL, 0);

  //printk("mapping .text\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)stext, (VirtAddr)etext, Identical, VM_R|VM_X), NULL, 0);
  //printk("mapping .rodata\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)srodata, (VirtAddr)erodata, Identical, VM_R), NULL, 0);
  //printk("mapping .data\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)sdata, (VirtAddr)edata, Identical, VM_R|VM_W), NULL, 0);
  //printk("mapping .bss\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)sbss_with_stack, (VirtAddr)ebss, Identical, VM_R|VM_W), NULL, 0);
  //printk("mapping ekernel~MEMORY_END\n");
    ms_push(KERNEL_SPACE, ma_new((VirtAddr)(&ekernel), (VirtAddr)MEMORY_END, Identical, VM_R|VM_W), NULL, 0);


  //printk("mapping memory-mapped registers\n");
    for (int i = 0; i < sizeof(k210_memmap)/sizeof(k210_memmap[0]); i++)
    {
        if (i != 1 && i != 4 && i != 3)
            ms_push(KERNEL_SPACE, ma_new((VirtAddr)k210_memmap[i].base, (VirtAddr)(k210_memmap[i].base+k210_memmap[i].size), Identical, VM_R|VM_W), NULL, 0);
        //printk("%d\n", i);
        //printk("%d\n", buddy_remain_size(HEAP_ALLOCATOR));
    }
}



void page_activate()
{
    new_kenel(&KERNEL_SPACE);
  //printk("kenel map done !\n");
    page_init();
  //printk("remain frame: [%d]\n", frame_remain_size(&FRAME_ALLOCATOR));
  //printk("remain kheap size: [%d]\n", buddy_remain_size(HEAP_ALLOCATOR));
    // 这条写入 satp 的指令及其下一条指令都在内核内存布局的代码段中，在切换之后是一个恒等映射， 而在切换之前是视为物理地址直接取指，也可以将其看成一个恒等映射。这完全符合我们的期待：即使切换了地址空间，指令仍应该 能够被连续的执行。
}

void page_init()
{
    uint64_t satp = token(&KERNEL_SPACE.page_table);
    csr_write(CSR_SATP, satp);
    //asm volatile("csrw satp, %0" : : "r" (satp));
  //printk("satp [%p]\n", satp);
  //printk("paging ... done!\n");
    //flush_tlb_all();
    do {
        asm volatile("fence");
        asm volatile("fence.i");
        asm volatile("sfence.vma" : : : "memory");
        asm volatile("fence");
        asm volatile("fence.i");
    } while(0);
  //printk("flush\n");
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



struct MemorySet *from_elf(uint8_t *elf_data, uint64_t datasize ,uint64_t *usp, uint64_t *entry)
{
    struct MemorySet *memset_set = (struct MemorySet *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct MemorySet));
    ms_init(memset_set);
    ms_map_trampoline(memset_set);

    

    struct elfhdr eh;
    memmove(&eh, elf_data, sizeof(eh));

  printk("Magic :[%p]\n", eh.magic);
  printk("Type :[%d]\n",eh.type);
  printk("Entry point address :[%p]\n",eh.entry);
  printk("Start of program headers :[%d]\n",eh.phoff);
  printk("Start of section headers :[%d]\n",eh.shoff);
  printk("Size of elf header [%d]\n",eh.ehsize);
  printk("Size of program headers: [%d]\n",eh.phsize);
  printk("Number of program headers :[%d]\n",eh.phnum);
  printk("Size of section headers :[%d]\n",eh.shsize);
  printk("Number of section headers :[%d]\n", eh.shnum);
  printk("Section header string table index :[%d]\n", eh.shstrndx);

    uint64_t i, off;
    struct proghdr ph;
    VirtPageNum max_end_vpn = 0;
    for (i = 0, off = eh.phoff; i < eh.phnum; i++, off += sizeof(ph))
    {
        memmove(&ph, elf_data + off, sizeof(ph));
        printk("Type (ELF_PROG_LOAD = 1) :[%d]\n", ph.type);
        if (ph.type == ELF_PROG_LOAD)
        {
          printk("FileSize :[%d]\n", ph.filesz);
          printk("MemSize :[%d]\n", ph.memsz);
          printk("VirtAddr :[%p]\n", ph.vaddr);
          printk("PhysAddr :[%p]\n", ph.paddr);
          printk("Flags :[%p]\n", ph.flags);
          printk("Align :[%p]\n", ph.align);
          printk("Offset :[%p]\n", ph.off);

            VirtAddr start_va = ph.vaddr;
            VirtAddr end_va = ph.vaddr + ph.memsz;

            MapPermission map_perm = VM_U;
            if (ph.flags & ELF_PROG_FLAG_EXEC)
                map_perm |= VM_X;
            if (ph.flags & ELF_PROG_FLAG_READ)
                map_perm |= VM_R;
            if (ph.flags & ELF_PROG_FLAG_WRITE)
                map_perm |= VM_W;
          //printk("perm :[%p]\n",map_perm);
            
            if(ph.memsz < ph.filesz)
                panic("memsize < filesize \n");
            
            struct MapArea *ma = ma_new(start_va, end_va, Framed, map_perm);
            max_end_vpn = ma->vm_end;
            printk("hi\n");
                uint64_t page_nums = FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current;
    printk("remain %d (%p ~ %p) Phys Frames \n", page_nums, FRAME_ALLOCATOR.current, FRAME_ALLOCATOR.end);
    printk("need: %d", va2vpn(end_va) - va2vpn(start_va));
            ms_push(memset_set, ma, elf_data + ph.off, ph.filesz); // 数据切片

        }   
    }
    VirtAddr max_end_va = vpn2va(max_end_vpn);
    printk("va :[%p]\n", max_end_va);

    uint64_t user_stack_bottom= max_end_va  + PAGE_SIZE;
    uint64_t user_stack_top = user_stack_bottom + USER_STACK_SIZE;

    // user_stack_top 没有映射
    ms_push(memset_set, ma_new(user_stack_bottom, user_stack_top+PAGE_SIZE, Framed, VM_R|VM_W|VM_U), NULL, 0);
        
    ms_push(memset_set, ma_new((VirtAddr)TRAP_CONTEXT, (VirtAddr)TRAMPOLINE, Framed ,VM_R|VM_W), NULL, 0);

    *usp = user_stack_top;
    printk("usp :[%p]\n", *usp);
    *entry = eh.entry;
    printk("entry :[%p]\n", *entry);

    return memset_set;

}


int mmap_handler(uint64_t va, int scause)
{
    //printk("va : [%p]\n", va);
    struct TaskControlBlock *p = mytask();
    struct MapArea *t = NULL;

    struct list_elem* elem = p->memory_set->vma.head.next;
    while(elem != &p->memory_set->vma.tail) {
        t = elem2entry(struct MapArea, tag, elem);
        
        // printk("found :[%p]\n", t);
        // printk("vs :[%p]\n",t->vm_start);
        // printk("ve :[%p]\n",t->vm_end);

        if(va >= vpn2va(t->vm_start) && va < vpn2va(t->vm_end)){
            break;
        }

        elem = elem->next;
    }
    

    if (elem == &p->memory_set->vma.tail) //不是mmap
    {
        printk("no mmap adar :[%p]\n", va);
        return -1;
    }
    
    if((scause == 13 || scause == 5) && !(t->map_perm & PTE_R)) return -2; // unreadable vma
    if((scause == 15 || scause == 7) && !(t->map_perm & PTE_W)) return -3; // unwritable vma

    //panic("mmap page fault\n");

    // 映射该页并分配内存
    ma_map_one(t, &p->memory_set->page_table, va2vpn(va)); //[0x0000000000040000]->[0x00000000000801a9]
    asm volatile("sfence.vma");

    //printk("%d\n", t->map_perm); // 22
    //printk("va-vpn [%p]\n", va2vpn(va));
    // 获取该页物理地址
    PhysAddr pa = pte_get_pa(*(find_pte(&p->memory_set->page_table, va2vpn(va))));

    //printk("va[%p] -> pa[%p]\n", va, pa);
    // 将文件内容写入该页
    //fileread(t->f, va, PAGE_SIZE);
    eread(t->f->ep, 0, (uint64_t)pa, t->off + va - vpn2va(t->vm_start), PAGE_SIZE);

    //printk("%s", (char *)pa);
    
//   iunlock(f->ip);
    return 0;

}

