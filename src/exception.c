#include "include/exception.h"
#include "include/riscv_asm.h"
#include "include/console.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/syscall.h"
#include "include/assert.h"
#include "include/mm.h"
#include "include/proc.h"

extern void __saveall(void);
extern void __restore(void);

extern uintptr_t boot_stack_top;





void idt_init(void)
{

    csr_write(CSR_SSCRATCH, TRAP_CONTEXT);
    //csr_write(CSR_SSCRATCH, 0);
    csr_write(CSR_STVEC, TRAMPOLINE);   // 由于函数地址四字节对其，所以设置后模式为Direct
    // 把 stvec 设置为内核和应用地址空间共享的跳板页面的起始地址 TRAMPOLINE 而不是 编译器在链接时看到的 __saveall 的地址
    // 因为启用分页模式之后我们只能通过跳板页面上的虚拟地址来实际取得 __saveall 和 __restore 的汇编代码。

    // csr_write(CSR_STVEC, &__saveall);
    //初始化　或者　 进入ｔｒａｐ时设置ｋｅｒｎｅｌ　ｔｒａｐ　ｅｎｔｒｙ，返回时设置　ｕｓｅｒ　ｔｒａｐ　ｅｎｔｒｙ
    //也可以在　ｔｒａｐ　ｅｎｔｒｙ　中判断　？
    // printk("__saveall : [%p]\n", &__saveall);
    // printk("set stvec : [%p] -> [%p]\n", TRAMPOLINE, pte_get_ppn(*(find_pte(&KERNEL_SPACE.page_table, va2vpn((VirtAddr)TRAMPOLINE)))));
    // printk("set trapcontex : [%p] -> [%p]\n", TRAP_CONTEXT, pte_get_ppn(*(find_pte(&KERNEL_SPACE.page_table, va2vpn((VirtAddr)TRAP_CONTEXT)))));
    // // printk("");
    // printk("__restore : [%p]\n", &__restore);
}


void irq_enable(void)
{
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
    //cons_puts("irq enable!\n");
}

void irq_disable(void)
{
    csr_clear(CSR_SSTATUS, SSTATUS_SIE);
    //cons_puts("irq disable\n");
}

// ir enable ?
bool irq_get(void)
{
    uint64_t s = csr_read(CSR_SSTATUS);
    return (s & SSTATUS_SIE) != 0;
}


void breakpoint(struct context *f)
{
    warn("Breakpoint at [%p]!\n", f->epc);
    f->epc += 2; //?
}

static inline void exception_dispatch(struct context *f)
{
    if ((intptr_t)f->cause < 0)
    {
        cons_puts("interrupt\n");
        //printk("I code :[%p]\n", f->cause);
        irq_handler(f);
    }
    else
    {
        cons_puts("sync_exception\n");
        //printk("E code :[%p]\n", f->cause);
        //printk("[%p]\n", f->epc);
        exc_handler(f);
    }
    
}


void e_return()
{
    // 在下降到Ｕ特权级时要注意sp

    uint64_t satp = mytask_token();//token(&KERNEL_SPACE.page_table); // current_task_token
    //asm volatile("fence");
    
    //warn("satp : [%p]", satp);
    // cons_puts("?"); //　会产生非法
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
    struct context *f = mytask_trap_cx(); 
    // 由于应用的 Trap 上下文不在内核地址空间，因此我们调用 current_trap_cx 来获取当前应用的 Trap 上下文的可变引用 而不是像之前那样作为参数传入

    //printk("%p\n", f->status);
    //printk("%p\n", csr_read(CSR_SSTATUS));

    // unsigned long spp = csr_read(CSR_SSTATUS) & SSTATUS_SPP;
    //printk("[%p]\n", spp);

    //uint64_t satp = token(&KERNEL_SPACE.page_table);

    // if (spp != 0)
    // {
    //     panic("trap from kernel !\n");
    //     //satp = token(&KERNEL_SPACE.page_table);
    // }
    // else
    // {
    //     //cons_puts("trap from user ! \n");
    // }

    if ((intptr_t)f->cause < 0)
    {
        irq_handler(f);
    }
    else
    {
        exc_handler(f);
    }

    __sync_synchronize();

    e_return();
}


void irq_handler(struct context *f)
{
    intptr_t c = (f->cause << 1) >> 1;     //去掉符号位
    //cons_puts("int\n");
    switch (c) 
    {
        case IRQ_S_TIMER:
            timer_handle();
            break;

        default:
            break;
    }
    //printk("done! \n");
    //f->epc = (uintptr_t)&__restore;
    //printk("sepc: [%p]\n", f->epc);

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
        
        case EXC_INST_ACCESS_FAULT:
        case EXC_STORE_PAGE_FAULT:
        case EXC_LOAD_PAGE_FAULT:
        case EXC_INST_PAGE_FAULT:
            printk("epc - %p  cause - [%p]\n", f->epc, f->cause);
            exit_current_and_run_next(-2);
        case EXC_ILLEGAL_INST:
            printk("illegal ins \n");
            exit_current_and_run_next(-3);
        case EXC_STORE_ACCESS_FAULT:
        case EXC_LOAD_ACCESS_FAULT:
            if(mmap_handler(f->tval, f->cause) != 0){
                printk("page fault\n");
                exit_current_and_run_next(-2);
            }
            break;

        default:
            panic("epc - %p  cause - [%p]\n", f->epc, f->cause);
            break;
    }
}


void set_sp(struct context *c, uintptr_t sp)
{
    c->gr.sp = sp;
}

void app_context_init(struct context *app, uintptr_t entry, uintptr_t u_sp, uintptr_t k_satp, uintptr_t k_sp, uintptr_t trap_handle)
{
    uintptr_t sstatus = csr_read(CSR_SSTATUS);
    sstatus &= ~SSTATUS_SPP; // ? 如果从用户态进入中断， sstatus 的 SPP 位被硬件设为 0    或者 下降特权级到用户态
    sstatus |= SSTATUS_SPIE; // enable interrupts in U

    memset(app, 0, sizeof(struct context));
    app->status = sstatus;
    app->gr.sp = u_sp;
    app->epc = entry;
    app->kernel_satp = k_satp;
    app->kernel_sp = k_sp;
    app->trap_handle = trap_handle;

}


