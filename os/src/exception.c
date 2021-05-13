#include "include/exception.h"
#include "include/riscv_asm.h"
#include "include/console.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/syscall.h"
#include "include/assert.h"
#include "include/mm.h"

extern void __saveall(void);
extern void __restore(void);

extern uintptr_t boot_stack_top;



struct context *kernelcon;


void idt_init(void)
{
    PhysPageNum f = frame_alloc(&FRAME_ALLOCATOR);
    kernelcon = (struct context *)ppn2pa(f);
    kernelcon->kernel_satp = token(&KERNEL_SPACE.page_table);
    kernelcon->kernel_sp = (uintptr_t)&boot_stack_top;
    kernelcon->trap_handle = (uintptr_t)&e_dispatch;
    map(&KERNEL_SPACE.page_table, va2vpn(TRAP_CONTEXT), f, PTE_R|PTE_W);

    

    csr_write(CSR_SSCRATCH, TRAP_CONTEXT);
    csr_write(CSR_STVEC, TRAMPOLINE);   // 由于函数地址四字节对其，所以设置后模式为Direct
    // 把 stvec 设置为内核和应用地址空间共享的跳板页面的起始地址 TRAMPOLINE 而不是 编译器在链接时看到的 __saveall 的地址
    // 因为启用分页模式之后我们只能通过跳板页面上的虚拟地址来实际取得 __saveall 和 __restore 的汇编代码。

    // csr_write(CSR_STVEC, &__saveall);
    //初始化　或者　 进入ｔｒａｐ时设置ｋｅｒｎｅｌ　ｔｒａｐ　ｅｎｔｒｙ，返回时设置　ｕｓｅｒ　ｔｒａｐ　ｅｎｔｒｙ
    //也可以在　ｔｒａｐ　ｅｎｔｒｙ　中判断　？
    printf("__saveall : [%p]\n", &__saveall);
    printf("set stvec : [%p] -> [%p]\n", TRAMPOLINE, pte_get_ppn(*(find_pte(&KERNEL_SPACE.page_table, va2vpn((VirtAddr)TRAMPOLINE)))));
    printf("set trapcontex : [%p] -> [%p]\n", TRAP_CONTEXT, pte_get_ppn(*(find_pte(&KERNEL_SPACE.page_table, va2vpn((VirtAddr)TRAP_CONTEXT)))));

    //printf("set restore : [%p]\n", &__restore);
}


void irq_enable(void)
{
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
    printf("irq enable!\n");
}

void irq_disable(void)
{
    csr_clear(CSR_SSTATUS, SSTATUS_SIE);
    printf("irq disable\n");
}

// ir enable ?
bool irq_get(void)
{
    uint64_t s = csr_read(CSR_SSTATUS);
    return (s & SSTATUS_SIE) != 0;
}


void breakpoint(struct context *f)
{
    printf("Breakpoint at [%p]!\n", f->epc);
    f->epc += 2; //?
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
        //printf("[%p]\n", f->epc);
        exc_handler(f);
    }
    
}


void e_return(uint64_t satp)
{
    // satp  要继续执行的应用 地址空间的 token
    //　Trap 上下文在应用地址空间中的虚拟地址 , 内核和应用都一样TRAP_CONTEXT
    uint64_t restore_va = &__restore - &__saveall + TRAMPOLINE;

    register uint64_t a0 asm ("a0") = (uint64_t)(TRAP_CONTEXT);
    register uint64_t a1 asm ("a1") = (uint64_t)(satp);


    asm volatile("fence.i");
    asm volatile("jr %0" :: "r"(restore_va), "r"(a0), "r"(a1));
    // ((void (*)(struct context *t, uint64_t))restore_va)(trap_cx_ptr, satp);
}


 void e_dispatch()
{
    struct context *f = (struct context *)TRAP_CONTEXT;  
    // 由于应用的 Trap 上下文不在内核地址空间，因此我们调用 current_trap_cx 来获取当前应用的 Trap 上下文的可变引用 而不是像之前那样作为参数传入

    //printf("%p\n", f->status);
    //printf("%p\n", csr_read(CSR_SSTATUS));

    unsigned long spp = csr_read(CSR_SSTATUS) & SSTATUS_SPP;
    //printf("[%p]\n", spp);

    uint64_t satp = token(&KERNEL_SPACE.page_table);

    if (spp != 0)
    {
        printf("trap from kernel !\n");
        satp = token(&KERNEL_SPACE.page_table);
    }
    else
        printf("trap from user !\n");

    exception_dispatch(f);
    e_return(satp);
}


void irq_handler(struct context *f)
{
    intptr_t cause = (f->cause << 1) >> 1;     //去掉符号位
    printf("%p\n",cause);
    switch (cause) 
    {
        case IRQ_S_TIMER:
            set_next_trigger();     //设置下一次时钟中断
            if (++TICKS % TICKS_PER_SEC == 0) {
                panic("it's time to sleep\n");
            }
            break;

        default:
            break;
    }
    //printf("done! \n");
    //f->epc = (uintptr_t)&__restore;
    //printf("sepc: [%p]\n", f->epc);

}


void exc_handler(struct context *f)
{
    switch (f->cause)
    {
        case EXC_BREAKPOINT:
            breakpoint(f);
            break;
        
        case EXC_SYSCALL:
            syscall_handler(f);
            break;

        default:
            break;
    }
}


void set_sp(struct context *c, uintptr_t sp)
{
    c->gr.sp = sp;
}

struct context *app_context_init(uintptr_t entry, uintptr_t u_sp, uintptr_t k_satp, uintptr_t k_sp, uintptr_t trap_handle)
{
    uintptr_t sstatus = csr_read(CSR_SSTATUS);
    sstatus |= SSTATUS_SPP;

    struct context *app = (struct context *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct context));
    memset(app, 0, sizeof(struct context));
    app->gr.sp = u_sp;
    app->epc = entry;
    app->kernel_satp = k_satp;
    app->kernel_sp = k_sp;
    app->trap_handle = trap_handle;


    return app;
}


