#include "include/exception.h"
#include "include/stddef.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/timer.h"
#include "include/assert.h"

int i;
extern uintptr_t skernel;
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



	printf("%s", "hello, world!\n");
	
	printf("kernel_start : 		[%p]\n", &skernel);
	printf(".text : 		[%p ~ %p]\n", &stext, &etext);
	printf(".rodata : 		[%p ~ %p]\n", &srodata, &erodata);
	printf(".data : 		[%p ~ %p]\n", &sdata, &edata);
	printf("boot_stack : 		[%p ~ %p]\n", &boot_stack, &boot_stack_top);
	printf(".bss : 			[%p ~ %p]\n", &sbss, &ebss);
	printf("kernel_end: 		[%p]\n", &ekernel);

	idt_init();
	irq_enable();
	asm volatile ("ebreak");
	printf(" ? \n");  //

	panic("goodbye %d\n", 1);


	//timer_init();
	//printf(" ? ");
	
	while(1) {}
}
