#include "include/exception.h"
#include "include/stddef.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/assert.h"
#include "include/mm.h"
#include "include/spinlock.h"

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

void main()
{
	memset(&sbss, 0, &ebss - &sbss); //清空bss段

	//printf("%p\n", r_mhartid());


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


	asm volatile ("ebreak");
	printf(" ? \n");  //

	struct spinlock l1;
	struct spinlock l2;

	initlock(&l1, "l1");

	acquire(&l1);
	//release(&l1);
	//acquire(&l1);
	//panic("unreachable !\n");



	//timer_init();
	//printf(" ? ");
	

	panic("shoutdown !\n");
	while(1) {}
}
