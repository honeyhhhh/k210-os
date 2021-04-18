	.section .text.entry
	.global _start
_start:

    # S态可以读取？mhartid
    #csrr  t0, mhartid             # read hardware thread id (`hart` stands for `hardware thread`)
    #bnez  t0, halt                   # run only on the first hardware thread (hartid == 0), halt all the other threads  将除了0核的hart停机


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