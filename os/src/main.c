#include "include/exception.h"
#include "include/stddef.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/assert.h"
#include "include/mm.h"
#include "include/spinlock.h"
#include "include/sbi.h"

int i;
extern uintptr_t skernel[];
extern uintptr_t stext;
extern uintptr_t etext;
extern uintptr_t srodata;
extern uintptr_t erodata;
extern uintptr_t sdata;
extern uintptr_t edata;
extern uintptr_t sbss;
extern uintptr_t ebss;
extern uintptr_t boot_stack;
extern uintptr_t boot_stack_top;
extern uintptr_t ekernel;

static inline void inithartid(uint64_t hartid)
{
	asm volatile("mv tp, %0"
				 :
				 : "r"(hartid & 0x1));
}
volatile static int started = 0;

void main(uint64_t hartid, uint64_t dtb_pa)
{
	inithartid(hartid);

	if (hartid == 0)
	{
		//memset(&sbss, 0, &ebss - &sbss); //清空bss段
		printinit();
		printf("in core : \033[31m[%d]\033[0m\n", hartid);
		printf("%s", "hello, world!\n");

		printf("kernel_start : 		\033[31m[%p]\033[0m\n", skernel);
		printf(".text : 		\033[31;43m[%p ~ %p]\033[0m\n", &stext, &etext);
		printf(".rodata : 		[%p ~ %p]\n", &srodata, &erodata);
		printf(".data : 		[%p ~ %p]\n", &sdata, &edata);
		printf("boot_stack : 		[%p ~ %p]\n", &boot_stack, &boot_stack_top);
		printf(".bss : 			[%p ~ %p]\n", &sbss, &ebss);
		printf("kernel_end: 		[%p]\n", &ekernel);

		mm_init();
		idt_init();
		irq_enable();
		__sync_synchronize();

		uint64_t vec = csr_read(CSR_STVEC);
		printf("%p\n",vec);

		printf("%d\n", irq_get());

		//irq_disable();
		//printf("%d\n", irq_get());
		printf("sp :%p\n", kernelcon->kernel_sp);
		printf("satp :%p\n", kernelcon->kernel_satp);
		printf("handle : %p\n", kernelcon->trap_handle);

		asm volatile ("ebreak");
		printf(" ? \n");
		asm volatile ("ebreak");
		printf(" ? \n");

		//struct spinlock l1;
		//struct spinlock l2;

		//initlock(&l1, "l1");

		//acquire(&l1);
		//release(&l1);
		//acquire(&l1);
		//panic("unreachable !\n");

		//printf(" ? ");
		for (int i = 1; i < NCPU; i++)
		{
			unsigned long mask = 1 << i;
			sbi_send_ipi(&mask);
		}
		__sync_synchronize();
		started = 1;

		// int p1 = irq_get();
		// printf("%d\n", p1);

		uint64_t x2;
		asm volatile("mv %0, tp"
					 : "=r"(x2));
		printf("tp :[%d]\n", x2);

		uint64_t s1;
		asm volatile("csrr %0, satp"
					 : "=r"(s1));
		printf("satp :[%p]\n", s1);



		//timer_init();


		//panic("shoutdown !\n");
	}
	else
	{
		//printf("in core : \033[31m[%d]\033[0m \n", hartid);

		while (started == 0)
			;
		__sync_synchronize();


		page_init();
		idt_init();

		uint64_t x1;
		asm volatile("mv %0, tp"
					 : "=r"(x1));

		printf("in core : \033[31m[%d]\033[0m  tp: [%d]\n", hartid, x1);

		uint64_t s2;
		asm volatile("csrr %0, satp"
					 : "=r"(s2));
		printf("satp :[%p]\n", s2);
		// int p2 = irq_get();
		// printf("%d\n", p2);
		
		uint64_t vec = csr_read(CSR_STVEC);
		printf("%p\n",vec);


		printf("%d\n", irq_get());




		//asm volatile ("ebreak");
		//printf(" ? \n");

		//sbi_shutdown();
		//panic("shutdown ! in core : \033[31m[%d]\033[0m\n", hartid);
	}
}
