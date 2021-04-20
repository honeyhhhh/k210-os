#include "include/exception.h"
#include "include/riscv_asm.h"
#include "include/console.h"
#include "include/stdio.h"
#include "include/timer.h"

extern void __saveall(void);
extern void __restore(void);

void idt_init(void)
{
    csr_write(CSR_SSCRATCH, 0);
    csr_write(CSR_STVEC, &__saveall);   // 由于函数地址四字节对其，所以设置后模式为Direct
    printf("set stvec : [%p]\n", &__saveall);
    printf("set restore : [%p]\n", &__restore);
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
        printf("I code :[%p]\n", f->cause);
        irq_handler(f);
    }
    else
    {
        cons_puts("sync_exception\n");
        printf("E code :[%p]\n", f->cause);
    }
    
}

void e_dispatch(struct context *f)
{
    exception_dispatch(f);
}


void irq_handler(struct context *f)
{
    intptr_t cause = (f->cause << 1) >> 1;     //去掉符号位
    printf("%d\n", cause);
    switch (cause) 
    {
        case IRQ_S_TIMER:
            set_next_trigger();     //设置下一次时钟中断
            if (++TICKS % TICKS_PER_SEC == 0) {
                printf("it's time to sleep\n");
            }
            break;

        default:
            break;
    }
    printf("done! \n");
    //f->epc = (uintptr_t)&__restore;
    printf("sepc: [%p]\n", f->epc);

}



