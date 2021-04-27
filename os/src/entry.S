	.section .text.entry
	.global _start
_start:

	la sp, boot_stack_top
	tail main

halt:		j halt 

	.section .data.kpt
	.align 12
	.global kernel_page_table_sv39
kernel_page_table_sv39:
	.zero 8 * 511


	.section .bss.stack
	.align 12
	.global boot_stack
boot_stack:
	.zero 4096 * 4
	.global boot_stack_top
boot_stack_top:


