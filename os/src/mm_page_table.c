#include "include/mm.h"
#include "include/string.h"



/* 取出虚拟页号的三级页索引 高到低 */
inline void vpn_indexes(VirtPageNum vpn, uintptr_t *index3)
{
    memset(index3, 0, 3*sizeof(uintptr_t));
    for (int i = 2; i >= 0; i++)
    {
        index3[i] = vpn & 511;
        vpn >>= 9;
    }
}

/* 访问一个特定物理页帧方法：先把物理页号转为物理地址 PhysAddr ，然后再转成 usize 形式的物理地址 */
/* 通过pnn返回页表项数组 , 用于根据当前页索引找到页表项 */
Pte *get_pte_array(PhysPageNum ppn) // 8字节粒度
{
    return (void *)ppn2pa(ppn);
}
uint8_t *get_byte_array(PhysPageNum ppn) // 1字节粒度
{
    return (void *)ppn2pa(ppn);
}

// 将相应页号和标志生成一个页表项
Pte pte_new(PhysPageNum ppn, PTEFlags flags)
{
    uint64_t pte = (ppn << 10) | (uint64_t)flags;
}
// 获取页号、地址、标志位
PhysPageNum pte_get_ppn(Pte pte)
{
    uint64_t t = ((uint64_t)1 << 44) - 1;
    return pte & t;
}
PhysAddr pte_get_pa(Pte pte)
{
    return ppn2pa(pte_get_ppn(pte));
}
PTEFlags pte_get_flags(Pte pte)
{
    return (uint8_t)(pte & 0x0FF);
}
// 清空
void pte_empty(Pte *pte)
{
    *pte = 0;
}
// 判断V、R、W、X
bool pte_is_valid(Pte pte)
{
    return (pte_get_flags(pte) & PTE_V) != 0;
}
bool pte_readable(Pte pte)
{
    return (pte_get_flags(pte) & PTE_R) != 0;
}
bool pte_writable(Pte pte)
{
    return (pte_get_flags(pte) & PTE_W) != 0;

}
bool pte_excutable(Pte pte)
{
    return (pte_get_flags(pte) & PTE_X) != 0;
}




void fvec_init(struct fvec **f, PhysPageNum root_pnn)
{
    *f = (struct fvec *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct fvec));
    (*f)->ppn = root_pnn;
    (*f)->next = *f;
}
void fvec_push(struct fvec *f, PhysPageNum pnn)
{
    struct fvec *t = NULL;
    struct fvec *n = (struct fvec *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct fvec));
    n->ppn = pnn;

    t = f->next;
    f->next = n;
    n->next = t;
}
void fvec_destory(struct fvec **f)
{
    struct fvec *t = (*f)->next;
    (*f)->next = NULL;
    (*f) = t;
    while (*f != NULL)  
    {
        t = t->next;
        buddy_free(HEAP_ALLOCATOR, *f);
        *f = t;
    }
    *f = NULL;
}





void pt_new(struct PageTable *pt)
{
    PhysPageNum frame = frame_alloc(&FRAME_ALLOCATOR);
    uint8_t *p = get_byte_array(frame);
    memset(p, 0, PAGE_SIZE);
    pt->root_ppn = frame;
    fvec_init(&pt->fnode, pt->root_ppn);
}
//struct pageTable *from_token(uint64_t sapt);
Pte *find_pte_create(struct PageTable *self, VirtPageNum vpn)
{
    uintptr_t idxs[3];
    vpn_indexes(vpn, idxs);
    PhysPageNum ppn = self->root_ppn;
    Pte *result = NULL;
    for (int i = 0; i < 3; i++)
    {
        Pte *pte = &get_pte_array(ppn)[idxs[i]];
        if (i == 2)
        {
            result = pte;
            break;
        }
        if (!pte_is_valid(*pte))
        {
            PhysPageNum frame = frame_alloc(&FRAME_ALLOCATOR);
            uint8_t *p = get_byte_array(frame);
            memset(p, 0, PAGE_SIZE);
            *pte = pte_new(frame, PTE_V);
            fvec_push(self->fnode, frame);
        }
        ppn = pte_get_ppn(*pte);
    }
    return result;
    
}
Pte *find_pte(struct PageTable *self, VirtPageNum vpn)
{
    uintptr_t idxs[3];
    vpn_indexes(vpn, idxs);
    PhysPageNum ppn = self->root_ppn;
    Pte *result = NULL;
    for (int i = 0; i < 3; i++)
    {
        Pte *pte = &get_pte_array(ppn)[idxs[i]];
        if (i == 2)
        {
            result = pte;
            break;
        }
        if (!pte_is_valid(*pte))
        {
            return NULL;
        }
        ppn = pte_get_ppn(*pte);
    }
    return result;
}
void map(struct PageTable *self, VirtPageNum vpn, PhysPageNum ppn, PTEFlags flags)
{
    Pte *pte = find_pte_create(self, vpn);
    assert(pte_is_valid(*pte));
    *pte = pte_new(ppn, flags | PTE_V);
}
void unmap(struct PageTable *self, VirtPageNum vpn)
{
    Pte *pte = find_pte_create(self, vpn);
    assert(pte_is_valid(*pte));
    pte_empty(pte);
}

























