#ifndef _MM_H
#define _MM_H

#include "stddef.h"
#include "assert.h"
#include "max_heap.h"
#include "bitmap.h"

#define RUSTSBI_BASE 0x80000000L
extern uintptr_t skernel;   //0x80200000L
extern uintptr_t ekernel;
#define MEMORY_END 0x80800000L
// [ekernel, MEMORY_END)
#define PAGE_SIZE 4096L
#define PAGE_OFFSET_BITS 12L


/* 地址和页号 */
typedef uintptr_t PhysAddr;
typedef uintptr_t PhysPageNum;
typedef uintptr_t VirtAddr;
typedef uintptr_t VirtPageNum;

/* 页表基地址 */
#define SATP_MODE 0xF000000000000000
#define SATP_ASID 0x0FFFF00000000000
#define SATP_PPN  0x00000FFFFFFFFFFF
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
/* 页号和地址的相互转换 */
static inline PhysPageNum pa2ppn(PhysAddr pa)
{
    assert(get_offset(pa) != 0);
    return pa_floor(pa);
}
static inline PhysAddr ppn2pa(PhysPageNum pnn)
{
    return pnn << PAGE_OFFSET_BITS;
}
static inline VirtPageNum va2vpn(VirtAddr va)
{
    return pa2ppn(PhysAddr(va));
}
static inline VirtAddr vpn2va(VirtPageNum vpn)
{
    return ppn2pa(PhysPageNum(vpn));
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
    return pa_floor(PhysAddr(va));
}
static inline VirtPageNum va_ceil(VirtAddr va)
{
    return pa_ceil(PhysAddr(va));
}


/* 取出虚拟页号的三级页索引 高到低 */
void vpn_indexes(VirtPageNum vpn, uintptr_t *index3);





/* 页表项 抽象数据结构 */
typedef uintptr_t Pte;
typedef uint8_t PTEFlags;
#define PTE_V     0x001 // Valid
#define PTE_R     0x002 // Read
#define PTE_W     0x004 // Write
#define PTE_X     0x008 // Execute
#define PTE_U     0x010 // User
#define PTE_G     0x020 // Global
#define PTE_A     0x040 // Accessed
#define PTE_D     0x080 // Dirty
//#define PTE_RSW   0x300 // Reserved for Softwar
// 将相应页号和标志写入一个页表项
Pte pte_new(PhysAddr, PTEFlags);
// 获取页号、地址、标志位
PhysPageNum pte_get_ppn();
PhysAddr pte_get_pa();
PTEFlags pte_get_flags();
// 清空
void pte_empty();
// 判断V、R、W、X
int pte_is_valid();
int pte_readable();
int pte_writable();
int pte_excutable();
/* 通过pnn返回页表项数组 , 用于根据当前页索引找到页表项 */
Pte *get_pte_array(PhysPageNum ppn);




/* 页表(页表项数组) 抽象数据结构 */
typedef struct PageTable {
    PhysPageNum root_ppn;
    Pde *frames;
}PageTable;
void pt_new();
Pte *find_pte_create();
void map();
void unmap();



// 
static inline void mm_init()
{
    heap_allocator_init();
    frame_allocator_init();
    page_activate();
}
/* 最大堆式物理页帧分配器，相比于栈式更容易分配连续的页  */
/* 伙伴系统分配算法也容易分配连续物理地址的页框，但是 */
struct FrameAllocator
{
    struct bitmap map;
    PhysPageNum current;
    PhysPageNum end;
    struct maxHeap recycled;
};

void heap_allocator_init();     // 给内核堆分配器 一块静态零初始化的字节数组 用于分配， 位于内核bss段
void frame_allocator_init();    // 可用物理页帧管理器，[ekernel, MEMORY_END)
void page_activate();           // 初始化satp，开启分页








#endif



