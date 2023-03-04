/* mostly come from linux-riscv document */
#ifndef _SBI_H
#define _SBI_H

#include "stddef.h"


#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

/*
//#define SBI_ECALL(__num, __a0, __a1, __a2)                           \
//({                                                                  \
//    register unsigned long a0 asm("a0") = (unsigned long)(__a0);    \
//    register unsigned long a1 asm("a1") = (unsigned long)(__a1);    \
//    register unsigned long a2 asm("a2") = (unsigned long)(__a2);    \
//    register unsigned long a7 asm("a7") = (unsigned long)(__num);   \
//    asm volatile("ecall"                                            \
//                 : "+r"(a0)                                         \
//                 : "r"(a1), "r"(a2), "r"(a7)                        \
//                 : "memory");                                       \
//    a0;                                                             \
//})

//#define SBI_ECALL_0(__num) SBI_ECALL(__num, 0, 0, 0)
//#define SBI_ECALL_1(__num, __a0) SBI_ECALL(__num, __a0, 0, 0)
//#define SBI_ECALL_2(__num, __a0, __a1) SBI_ECALL(__num, __a0, __a1, 0)

//#define SBI_PUTCHAR(__a0) SBI_ECALL_1(SBI_CONSOLE_PUTCHAR, __a0)
//#define SBI_TIMER(__a0) SBI_ECALL_1(SBI_SET_TIMER, __a0)

*/

struct sbiret {
	long error;
	long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5);


void sbi_console_putchar(int ch);
int sbi_console_getchar(void);
void sbi_shutdown(void);
void sbi_send_ipi(const unsigned long *hart_mask);

void set_timer(uint64_t stime_value);












#endif
