#ifndef _MM_H
#define _MM_H

#include "stddef.h"
#include "assert.h"
#include "max_heap.h"
#include "bitmap.h"
#include "bitmap_buddy.h"
#include "spinlock.h"
#include "exception.h"
#include "list.h"
#include "file.h"

#define RUSTSBI_BASE 0x80000000L
extern uintptr_t skernel[]; //0x80200000L
#define KERNEL_HEAP_SIZE (4096 * 1024)
#define KERNEL_STACK_SIZE (4096 * 4)
#define USER_STACK_SIZE (4096 * 2)
extern uintptr_t ekernel;
#define MEMORY_END 0x80800000L
// [ekernel, MEMORY_END)
#define PAGE_SIZE 4096L
#define PAGE_OFFSET_BITS 12L




#define TRAMPOLINE (ULONG_MAX - PAGE_SIZE + 1)
#define TRAP_CONTEXT (TRAMPOLINE - PAGE_SIZE)



#define VMA_START 0x40000000














/* 地址和页号 */
typedef uintptr_t PhysAddr;
typedef uintptr_t PhysPageNum;
typedef uintptr_t VirtAddr;
typedef uintptr_t VirtPageNum;

/* 页表基地址 */
#define SATP_MODE 0xF000000000000000
#define SATP_ASID 0x0FFFF00000000000
#define SATP_PPN 0x00000FFFFFFFFFFF
typedef uintptr_t Pde;

/* 根据地址得到页内偏移，判断是否4K对齐 */
static inline uintptr_t get_offset(uintptr_t a)
{
    return a & (PAGE_SIZE - 1);
}
static inline bool is_aligned(uintptr_t a)
{
    return get_offset(a) == 0;
}
// floor向下 ceil向上
static inline PhysPageNum pa_floor(PhysAddr pa)
{
    return pa / PAGE_SIZE;
}
static inline PhysPageNum pa_ceil(PhysAddr pa)
{
    return (pa + PAGE_SIZE - 1) / PAGE_SIZE;
}
static inline VirtPageNum va_floor(VirtAddr va)
{
    return pa_floor((PhysAddr)va);
}
static inline VirtPageNum va_ceil(VirtAddr va)
{
    return pa_ceil((PhysAddr)va);
}
/* 页号和地址的相互转换 */
static inline PhysPageNum pa2ppn(PhysAddr pa)
{
    //assert(get_offset(pa) == 0);
    return pa_floor(pa);
}
static inline PhysAddr ppn2pa(PhysPageNum pnn)
{
    return pnn << PAGE_OFFSET_BITS;
}
static inline VirtPageNum va2vpn(VirtAddr va)
{
    return pa2ppn((PhysAddr)va);
}
static inline VirtAddr vpn2va(VirtPageNum vpn)
{
    return ppn2pa((PhysPageNum)vpn);
}

/* 取出虚拟页号的三级页索引 高到低 */
void vpn_indexes(VirtPageNum vpn, uintptr_t *index3);

/* 页表项 抽象数据结构 */
typedef uintptr_t Pte;
typedef uint8_t PTEFlags;
#define PTE_V 0x001 // Valid
#define PTE_R 0x002 // Read
#define PTE_W 0x004 // Write
#define PTE_X 0x008 // Execute
#define PTE_U 0x010 // User
#define PTE_G 0x020 // Global
#define PTE_A 0x040 // Accessed
#define PTE_D 0x080 // Dirty
//#define PTE_RSW   0x300 // Reserved for Softwar
// 将相应页号和标志写入一个页表项
Pte pte_new(PhysAddr, PTEFlags);
// 获取页号、地址、标志位
PhysPageNum pte_get_ppn(Pte pte);
PhysAddr pte_get_pa(Pte pte);
PTEFlags pte_get_flags(Pte pte);
// 清空
void pte_empty(Pte *pte);
// 判断V、R、W、X
int pte_is_valid(Pte pte);
int pte_readable(Pte pte);
int pte_writable(Pte pte);
int pte_excutable(Pte pte);
/* 通过pnn返回页表项数组 , 用于根据当前页索引找到页表项 */
Pte *get_pte_array(PhysPageNum ppn);
uint8_t *get_byte_array(PhysPageNum ppn);


struct fvec
{
    PhysPageNum ppn;
    struct fvec *next;
};
// void fvec_init(struct fvec **f, PhysPageNum root_pnn);
// void fvec_push(struct fvec *f, PhysPageNum pnn);
// void fvec_destory(struct fvec **f);

/* 页表(页表项数组) 抽象数据结构 */
struct PageTable
{
    PhysPageNum root_ppn; // 根节点
    struct fvec *fnode;    // 包括根节点的所有页表结点的物理页帧,方便回收
};
void pt_new(struct PageTable *pt);
void pt_free(struct PageTable *pt);

//struct pageTable *from_token(uint64_t sapt);
Pte *find_pte_create(struct PageTable *self, VirtPageNum vpn);
Pte *find_pte(struct PageTable *self, VirtPageNum vpn);
void map(struct PageTable *self, VirtPageNum vpn,PhysPageNum ppn, PTEFlags flags);
void unmap(struct PageTable *self, VirtPageNum vpn);
uint64_t token(struct PageTable *self);

PhysAddr va2pa(VirtAddr va);// 以当前进程的页表


/* TLB: Flush all or one page from local TLB */
static inline void flush_tlb_all(void)
{
	__asm__ __volatile__ ("sfence.vma" : : : "memory");
}

static inline void flush_tlb_page(uintptr_t addr) //va ?
{
	__asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}



/* 最大堆式物理页帧分配器，相比于栈式更容易分配连续的页  */
/* 伙伴系统分配算法也容易分配连续物理地址的页框，但是 */
struct FrameAllocator
{
    struct spinlock lock;
    struct bitmap map;
    PhysPageNum current;
    PhysPageNum end;
    struct maxHeap recycled;
};
PhysPageNum frame_alloc(struct FrameAllocator *self);
void frame_dealloc(struct FrameAllocator *self, PhysPageNum ppn);
uint64_t frame_remain_size(struct FrameAllocator *self);


extern struct FrameAllocator FRAME_ALLOCATOR;
extern struct bitmap_buddy* HEAP_ALLOCATOR;
extern struct bitmap_buddy* HEAP_ALLOCATOR;

void heap_allocator_init();  // 给内核堆分配器 一块静态零初始化的字节数组 用于分配， 位于内核bss段
void frame_allocator_init(); // 可用物理页帧管理器，[ekernel, MEMORY_END)



#define K210_CLINT 0
#define QEMU_PLIC  1
#define K210_PLIC  2
#define QEMU_UART0 3
#define QEMU_VIRTIO0 4
#define K210_UARTHS 5
#define K210_GPIOHS 6
#define K210_GPIO 7
#define K210_SPI_SLAVE 8
#define K210_FPIOA 9
#define K210_TIMER0 10
#define K210_TIMER1 11
#define K210_TIMER2 12
#define K210_SYSCTL 14
#define K210_SPI0 15
#define K210_SPI1 16
#define K210_SPI2 17
#define K210_DMAC 18
#define K210_RTC 19

/* 地址空间抽象： */
// 逻辑段：一段连续地址的虚拟内存
// 地址空间：一系列有关联的逻辑段
#define Identical   0
#define Framed      1
#define EMPTY       2
typedef uint32_t MapType;
typedef uint8_t MapPermission;
#define VM_R    (1<<1)
#define VM_W    (1<<2)
#define VM_X    (1<<3)
#define VM_U    (1<<4)

struct vpmap
{
    VirtPageNum vpn;
    PhysPageNum ppn;
    struct vpmap *next;
};

struct MapArea
{
    // struct spinlock lock;
    MapType map_type;
    MapPermission map_perm;
    VirtPageNum vm_start;
    VirtPageNum vm_end;

    // 一个逻辑段内虚拟页号是连续的
    // 连接该逻辑段　所有的Framed的　虚拟页号-物理页号， , 方便回收
    // 有头结点
    struct vpmap *dataframe;
    struct vpmap *dataframe_tail;

    struct list_elem tag;


    //for mmap
    struct file *f;
    uint64_t off;
    uint64_t length;
    int flags;

};
struct MapArea *ma_new(VirtAddr start_va, VirtAddr end_va, MapType map_type, MapPermission map_perm);
void vpmap_push(struct vpmap **tail, struct vpmap *node);
struct vpmap *vpmap_remove(struct vpmap *head, VirtPageNum node);

struct MapArea *from_another(struct MapArea *a);
void ma_map_one(struct MapArea *self, struct PageTable *page_table, VirtPageNum vpn);

struct MemorySet
{
    struct PageTable page_table;

    struct list vma;
    struct list areas;
};
extern struct MemorySet KERNEL_SPACE;
void new_kernel();
void page_activate();           // 初始化satp，开启分页
void page_init();
void insert_framed_area(struct MemorySet *self, VirtAddr start_va, VirtAddr end_va, MapPermission perm);
struct MemorySet *from_existed_user(struct MemorySet *a);
void brk(uint64_t n);


void ms_init(struct MemorySet *self);
void ms_map_uinit(struct MemorySet *ums, uint8_t *initcode, uint32_t icsize, struct context *trap_cx, uint32_t cxsize);
void ms_map_trampoline(struct MemorySet *self);
void ms_ma_free(struct MemorySet *self);
void ms_free(struct MemorySet *self);



struct MemorySet *from_elf(uint8_t *elf_data, uint64_t datasize,uint64_t *usp, uint64_t *entry);
int mmap_handler(uint64_t va, int scause);


extern struct context* kernelcon;




int copyin2(char *dst, uint64_t srcva, uint64_t len);
int copyout2(uint64_t dstva, char *src, uint64_t len);
int copyinstr2(char *dst, uint64_t srcva, uint64_t max);






//#define VIRT_OFFSET             0x3F00000000L
#define VIRT_OFFSET             0x0L

#define UART                    0x38000000L

#define UART_V                  (UART + VIRT_OFFSET)

#define CLINT                   0x02000000L
#define CLINT_V                 (CLINT + VIRT_OFFSET)

#define PLIC                    0x0c000000L
#define PLIC_V                  (PLIC + VIRT_OFFSET)

#define PLIC_PRIORITY           (PLIC_V + 0x0)
#define PLIC_PENDING            (PLIC_V + 0x1000)
#define PLIC_MENABLE(hart)      (PLIC_V + 0x2000 + (hart) * 0x100)
#define PLIC_SENABLE(hart)      (PLIC_V + 0x2080 + (hart) * 0x100)
#define PLIC_MPRIORITY(hart)    (PLIC_V + 0x200000 + (hart) * 0x2000)
#define PLIC_SPRIORITY(hart)    (PLIC_V + 0x201000 + (hart) * 0x2000)
#define PLIC_MCLAIM(hart)       (PLIC_V + 0x200004 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart)       (PLIC_V + 0x201004 + (hart) * 0x2000)

#ifndef QEMU
#define GPIOHS                  0x38001000
#define DMAC                    0x50000000
#define GPIO                    0x50200000
#define SPI_SLAVE               0x50240000
#define FPIOA                   0x502B0000
#define SPI0                    0x52000000
#define SPI1                    0x53000000
#define SPI2                    0x54000000
#define SYSCTL                  0x50440000

#define GPIOHS_V                (0x38001000 + VIRT_OFFSET)
#define DMAC_V                  (0x50000000 + VIRT_OFFSET)
#define GPIO_V                  (0x50200000 + VIRT_OFFSET)
#define SPI_SLAVE_V             (0x50240000 + VIRT_OFFSET)
#define FPIOA_V                 (0x502B0000 + VIRT_OFFSET)
#define SPI0_V                  (0x52000000 + VIRT_OFFSET)
#define SPI1_V                  (0x53000000 + VIRT_OFFSET)
#define SPI2_V                  (0x54000000 + VIRT_OFFSET)
#define SYSCTL_V                (0x50440000 + VIRT_OFFSET)


#endif






































//
static inline void mm_init()
{
    heap_allocator_init();
    frame_allocator_init();
    page_activate();
}














#endif
