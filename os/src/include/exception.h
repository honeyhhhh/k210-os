#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "stddef.h"

struct regs {
    uintptr_t zero;  // Hard-wired zero     x0
    uintptr_t ra;    // Return address      x1
    uintptr_t sp;    // Stack pointer       x2
    uintptr_t gp;    // Global pointer      x3
    uintptr_t tp;    // Thread pointer      x4
    uintptr_t t0;    // Temporary           x5
    uintptr_t t1;    // Temporary           x6
    uintptr_t t2;    // Temporary           x7
    uintptr_t s0;    // Saved register/frame pointer    x8
    uintptr_t s1;    // Saved register                  x9
    uintptr_t a0;    // Function argument/return value  x10
    uintptr_t a1;    // Function argument/return value  x11
    uintptr_t a2;    // Function argument               x12
    uintptr_t a3;    // Function argument               x13
    uintptr_t a4;    // Function argument               x14
    uintptr_t a5;    // Function argument               x15
    uintptr_t a6;    // Function argument               x16
    uintptr_t a7;    // Function argument               x17
    uintptr_t s2;    // Saved register                  x18
    uintptr_t s3;    // Saved register                  x19
    uintptr_t s4;    // Saved register                  x20
    uintptr_t s5;    // Saved register                  x21
    uintptr_t s6;    // Saved register                  x22
    uintptr_t s7;    // Saved register                  x23
    uintptr_t s8;    // Saved register                  x24
    uintptr_t s9;    // Saved register                  x25
    uintptr_t s10;   // Saved register                  x26
    uintptr_t s11;   // Saved register                  x27
    uintptr_t t3;    // Temporary           x28                  
    uintptr_t t4;    // Temporary           x29
    uintptr_t t5;    // Temporary           x30
    uintptr_t t6;    // Temporary           x31
};


// 36个   8字节 
// sp 指向 从低地址到高地址 依次为  x0-x31和 4个csr寄存器，通过a0传入参数
struct context {
    struct regs gr;
    uintptr_t status; //sstatus
    uintptr_t epc; //sepc
    uintptr_t tval; //stval
    uintptr_t cause; //scause
};



void irq_enable(void);
void irq_disable(void);

void idt_init(void);

static inline void exception_dispatch(struct context *f);
struct context *e_dispatch(struct context *f);
void irq_handler(struct context *f);
void exc_handler(struct context *f);



#endif