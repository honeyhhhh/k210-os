#ifndef _MM_H
#define _MM_H

#include "stddef.h"

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


/* 根据物理地址得到页内偏移 */
uintptr_t get_page_offset(PhysAddr pa);
/* 页号和地址的相互转换 */
void pa2pnn(PhysAddr pa, PhysPageNum *pnn);
void pnn2pa(PhysPageNum pnn, PhysAddr *pa);
void pa_floor(PhysAddr *pa);
void pa_ceil(PhysAddr *pa);
/* 取出虚拟页号的三级页索引 */
uintptr_t *vpn_indexes();




/* 页表项 抽象数据结构 */
typedef uintptr_t Pte;
typedef uintptr_t PTEFlags;
#define PTE_V     0x001L // Valid
#define PTE_R     0x002L // Read
#define PTE_W     0x004L // Write
#define PTE_X     0x008L // Execute
#define PTE_U     0x010L // User
#define PTE_G     0x020L // Global
#define PTE_A     0x040L // Accessed
#define PTE_D     0x080L // Dirty
#define PTE_RSW   0x300L // Reserved for Software
// 将相应页号和标志写入一个页表项
Pte pte_new(PhysAddr, PTEFlags);
// 获取页号、地址、标志位
PhysPageNum pte_get_pnn();
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
Pte *get_pte_array();




/* 页表(页表项数组) 抽象数据结构 */
typedef struct PageTable {
    PhysPageNum root_pnn;
    Pde *frames;
}PageTable;
void pt_new();
Pte *find_pte_create();
void map();
void unmap();
uintptr_t token();




void mm_init();
void heap_init();
void frame_allocator_init();
void page_activate();








#endif



