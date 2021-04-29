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

/* 通过pnn返回页表项数组 , 用于根据当前页索引找到页表项 */
Pte *get_pte_array(PhysPageNum ppn)
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
    uint64_t t = (1 << 44) - 1;
    return pte & t;
}
PhysAddr pte_get_pa(Pte pte)
{
    return ppn2pa(pte_get_ppn(pte));
}
PTEFlags pte_get_flags(Pte pte)
{
    return (uint8_t)(pte & 0x0FF)
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
bool pte_readable()
{
    return (pte_get_flags(pte) & PTE_R) != 0;
}
bool pte_writable()
{
    return (pte_get_flags(pte) & PTE_W) != 0;

}
bool pte_excutable()
{
    return (pte_get_flags(pte) & PTE_X) != 0;
}