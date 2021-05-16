	.section .text.entry
	.global _start
_start:

	# csrr a1, mhartid
	la sp, boot_stack_top
	tail main

halt:		j halt 


	.section .bss.stack
	.align 12
	.global boot_stack
boot_stack:
	.zero 4096 * 4
	.global boot_stack_top
boot_stack_top:

	.section .rodata
	.global HEAP_ALLOCATOR
HEAP_ALLOCATOR:


