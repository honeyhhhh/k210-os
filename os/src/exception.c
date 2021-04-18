#include "include/exception.h"
#include "include/riscv_asm.h"
#include "include/console.h"

void idt_init(void)
{
    extern void __saveall(void);


    csr_write(CSR_SSCRATCH, 0);
    csr_write(CSR_STVEC, &__saveall);
}


void irq_enable(void)
{
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
}

void irq_disable(void)
{
    csr_clear(CSR_SSTATUS, SSTATUS_SIE);
}

static inline void exception_dispatch(struct context *f)
{
    if ((intptr_t)f->cause < 0)
    {
        cons_puts("interrupt\n");
    }
    else
    {
        cons_puts("sync_exception\n");
    }
    
}

void e_dispatch(struct context *f)
{
    exception_dispatch(f);
}



