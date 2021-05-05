#include "include/mm.h"
#include "include/riscv_asm.h"


struct MemorySet KERNEL_SPACE;





void page_activate()
{
    uint64_t satp = token(&KERNEL_SPACE.page_table);
    csr_write(CSR_SATP, satp);
    flush_tlb_all();
}

