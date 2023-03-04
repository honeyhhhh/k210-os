#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include "stddef.h"

#ifndef __ASSEMBLY__

#define nop()		__asm__ __volatile__ ("nop")

#define RISCV_FENCE(p, s) \
	__asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory. */
#define mb()		RISCV_FENCE(iorw,iorw)
#define rmb()		RISCV_FENCE(ir,ir)
#define wmb()		RISCV_FENCE(ow,ow)

/* These barriers do not need to enforce ordering on devices, just memory. */
#define __smp_mb()	RISCV_FENCE(rw,rw)
#define __smp_rmb()	RISCV_FENCE(r,r)
#define __smp_wmb()	RISCV_FENCE(w,w)

#endif


#define RISCV_ACQUIRE_BARRIER		"\tfence r , rw\n"
#define RISCV_RELEASE_BARRIER		"\tfence rw,  w\n"

//struct cpu;
// Mutual exclusion lock.
struct spinlock {
    uint locked;       // Is the lock held? 1

    // For debugging:
    char *name;        // Name of lock.
    struct Processor *cpu;   // The cpu holding the lock.
};

void initlock(struct spinlock *lk, char *name);
void push_off(void);
void pop_off(void);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
bool holding(struct spinlock *lk);




/*  sem 实现参考 */
// struct semaphore {
//     int value;
//     struct spinlock lock;
//     //struct proc *queue[NPROC];
//     int end;
//     int start;
// };

// void sem_init(struct semaphore *s, int value) {
//     s->value = value;
//     initlock(&s->lock, "semaphore_lock");
//     s->end = s->start = 0;
// }

// void sem_wait(struct semaphore *s) {
//     acquire(&s->lock);
//     s->value--;
//     if (s->value < 0) {
//         s->queue[s->end] = myproc();
//         s->end = (s->end + 1) % NPROC;
//         sleep(myproc(), &s->lock)
//     }
//     release(&s->lock);
// }

// void sem_signal(struct semaphore *s) {
//     acquire(&s->lock);
//     s->value++;
//     if (s->value <= 0) {
//         wakeup(s->queue[s->start]);
//         s->queue[s->start] = 0;
//         s->start = (s->start + 1) % NPROC;
//     }
//     release(&s->lock);
// };

#endif