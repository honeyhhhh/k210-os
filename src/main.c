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
#include "include/buf.h"
#include "include/fs.h"
#include "include/console.h"
#include "include/rtc.h"
#include "include/sysctl.h"

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

// a0 和　a1 由　rustsbi　填充
void main(uint64_t hartid, uint64_t dtb_pa)
{
	inithartid(hartid);

	if (hartid == 0)
	{
		memset(&sbss, 0, &ebss - &sbss); //清空bss段
		consoleinit();
		irq_enable();

		printinit();
		procinit();

		printk("bss: [%p] \n", &sbss);
		printk("kenel end [%p]\n", &ekernel);

		// mm
		mm_init();

		// trap
		idt_init();
		timer_init();
		csr_set(CSR_SIE, IE_EIE|IE_SIE|IE_TIE);
		//set_next_trigger();

		// 开启8M内存
		sysctl_pll_enable(SYSCTL_PLL1);
		sysctl_clock_enable(SYSCTL_CLOCK_PLL1);




		// fs
		fpioa_pin_init();
		dmac_init();
		disk_init();
		binit();
		fat32_init();
		fileinit();



		// task
		add_initproc();
		//printk("%p -> %p \n", 0xfffffffffffff088,va2pa((VirtAddr)0xfffffffffffff06e)); //sfence.vma

		file_search("/");

	
		task_run();
    	panic("Unreachable in main!");



	

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
		printk("tp :[%d]\n", x2);

		uint64_t s1;
		asm volatile("csrr %0, satp"
					 : "=r"(s1));
		printk("satp :[%p]\n", s1);





		
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
		printk("SIE: %p->%p\n", ie1,ie2);
		printk("sstatus: %p->%p\n", sst1,sst2);
		printk("sscratch: %p->%p\n", ssc1,ssc2);



		uint64_t x1;
		asm volatile("mv %0, tp"
					 : "=r"(x1));

		printk("in core : \033[31m[%d]\033[0m  tp: [%d]\n", hartid, x1);

		uint64_t s2;
		asm volatile("csrr %0, satp"
					 : "=r"(s2));
		printk("satp :[%p]\n", s2);

		
		uint64_t vec = csr_read(CSR_STVEC);
		printk("%p\n",vec);









		__sync_synchronize();


		asm volatile ("ebreak");
		asm volatile ("ebreak");
		asm volatile ("ebreak");


		printk("?\n");

		
		while (1);
		
	}
}
