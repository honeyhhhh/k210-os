#include "include/exception.h"
#include "include/stddef.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/assert.h"
#include "include/mm.h"
#include "include/spinlock.h"
#include "include/sbi.h"
#include "include/proc.h"
#include "include/sdcard.h"
#include "include/disk.h"
#include "include/fpioa.h"
#include "include/dmac.h"

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
		memset(&sbss, 0, &ebss - &sbss); //清空bss段
		printinit();








		uint64_t ie1 = csr_read(CSR_SIE);
		uint64_t sst1 = csr_read(CSR_SSTATUS);
		uint64_t ssc1 = csr_read(CSR_SSCRATCH);
		uint64_t vec1 = csr_read(CSR_STVEC);

		printf("in core : \033[31m[%d]\033[0m\n", hartid);
		printf("%s", "hello, world!\n");

		printf("kernel_start : 		\033[31m[%p]\033[0m\n", skernel);
		printf(".text : 		\033[31;43m[%p ~ %p]\033[0m\n", &stext, &etext);
		printf(".rodata : 		[%p ~ %p]\n", &srodata, &erodata);
		printf(".data : 		[%p ~ %p]\n", &sdata, &edata);
		printf("boot_stack : 		[%p ~ %p]\n", &boot_stack, &boot_stack_top);
		printf(".bss : 			[%p ~ %p]\n", &sbss, &ebss);
		printf("kernel_end: 		[%p]\n", &ekernel);


	
		__sync_synchronize();

		// mm
		mm_init();

		// trap
		idt_init();
		irq_enable();
		csr_set(CSR_SIE, IE_EIE|IE_SIE|IE_TIE);
		__sync_synchronize();




		uint64_t ie2 = csr_read(CSR_SIE);
		uint64_t sst2 = csr_read(CSR_SSTATUS);
		uint64_t ssc2 = csr_read(CSR_SSCRATCH);
		printf("SIE: %p->%p\n", ie1,ie2);
		printf("sstatus: %p->%p\n", sst1,sst2);
		printf("sscratch: %p->%p\n", ssc1,ssc2);

		uint64_t vec2 = csr_read(CSR_STVEC);
		printf("stvec: %p->%p\n",vec1,vec2);


		printf("sp :%p\n", kernelcon->kernel_sp);
		printf("satp :%p\n", kernelcon->kernel_satp);
		printf("handle : %p\n", kernelcon->trap_handle);

		// asm volatile ("ebreak");
		// printf(" ? \n");
		// asm volatile ("ebreak");
		// printf(" ? \n");


		procinit();
		// fs
		fpioa_pin_init();
		dmac_init();
		disk_init();
		test_sdcard();

		// task
		add_initproc();  // add_task()




		






		while (1);
	

		for (int i = 1; i < NCPU; i++)
		{
			unsigned long mask = 1 << i;
			sbi_send_ipi(&mask);
		}
		__sync_synchronize();
		started = 1;


		uint64_t x2;
		asm volatile("mv %0, tp"
					 : "=r"(x2));
		printf("tp :[%d]\n", x2);

		uint64_t s1;
		asm volatile("csrr %0, satp"
					 : "=r"(s1));
		printf("satp :[%p]\n", s1);




		//timer_init();

		
	}
	else
	{

		while (started == 0)
			;
		__sync_synchronize();

		uint64_t sst1 = csr_read(CSR_SSTATUS);
		uint64_t ssc1 = csr_read(CSR_SSCRATCH);
		uint64_t ie1 = csr_read(CSR_SIE);

		page_init();
		idt_init();
		csr_set(CSR_SIE, IE_EIE|IE_SIE|IE_TIE);
		__sync_synchronize();

		uint64_t sst2 = csr_read(CSR_SSTATUS);
		uint64_t ssc2 = csr_read(CSR_SSCRATCH);
		uint64_t ie2 = csr_read(CSR_SIE);
		printf("SIE: %p->%p\n", ie1,ie2);
		printf("sstatus: %p->%p\n", sst1,sst2);
		printf("sscratch: %p->%p\n", ssc1,ssc2);



		uint64_t x1;
		asm volatile("mv %0, tp"
					 : "=r"(x1));

		printf("in core : \033[31m[%d]\033[0m  tp: [%d]\n", hartid, x1);

		uint64_t s2;
		asm volatile("csrr %0, satp"
					 : "=r"(s2));
		printf("satp :[%p]\n", s2);

		
		uint64_t vec = csr_read(CSR_STVEC);
		printf("%p\n",vec);









		__sync_synchronize();


		asm volatile ("ebreak");
		asm volatile ("ebreak");
		asm volatile ("ebreak");


		printf("?\n");

		
		while (1);
		
	}
}
