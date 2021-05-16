#include "include/spinlock.h"
#include "include/exception.h"
#include "include/assert.h"
#include "include/console.h"


struct spinlock lock_locks;

// __sync_lock_test_and_set
static inline uint atomic_xchg(struct spinlock *v, int n)
{
    register uint c;

    __asm__ __volatile__ (
            "amoswap.w %0, %2, %1"
            : "=r" (c), "+A" (v->locked)
            : "r" (n)
            : "memory");
    return c;
}

// 可以在最后一条语句之前加入一个memory barrier,强制cpu执行完前面的写入以后再执行最后一条：
// memory barrier有几种类型：
// acquire barrier : 不允许将barrier之后的内存读取指令移到barrier之前（linux kernel中的wmb()）。
// release barrier : 不允许将barrier之前的内存读取指令移到barrier之后 (linux kernel中的rmb())。
// full barrier    : 以上两种barrier的合集(linux kernel中的mb())。


static inline uint spin_trylock(struct spinlock *lock)
{
	uint tmp = 1, busy;

	__asm__ __volatile__ (
		"	amoswap.w %0, %2, %1\n"
		RISCV_ACQUIRE_BARRIER
		: "=r" (busy), "+A" (lock->locked)
		: "r" (tmp)
		: "memory");

	return !busy;
}


static inline void memb(void)
{
    __asm__ __volatile__ ("fence");
}

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
}

bool holding(struct spinlock *lk)
{
    int r;
    r = (lk->locked);// && lk->cpu == mycpu());
    return r;
}


void acquire(struct spinlock *lk)
{
    //warn("%s want to get lock!\n", lk->name);

    // 1 disable interrupts to avoid deadlock.
    push_off();
    // 2 Check whether this cpu is holding the lock.

    // 3
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;

    // 4  mb()  fence 
    __sync_synchronize();

    //warn("%s get lock!\n", lk->name);

    // 5 Record info about lock acquisition for holding() and debugging.

}
void release(struct spinlock *lk)
{
    // 1 Check whether this cpu is holding the lock.

    // 2 lk->cpu = 0

    // 3
    __sync_synchronize();

    // 4
    __sync_lock_release(&lk->locked);

    //warn("%s release lock!\n", lk->name);

    // 5 enable ir
    pop_off();
}

void push_off(void)
{
    //int old = irq_get(); //是否开启中断

    irq_disable();
    // if(mycpu()->noff == 0)
    //     mycpu()->intena = old;  //inetna 表示在push off之前是否开启中断
    // mycpu()->noff += 1;          // Depth of push_off() nesting 嵌套层数
}

void pop_off(void)
{
    // struct cpu *c = mycpu();
    // if(irq_get())
    //     panic("pop_off - interruptible");
    // if(c->noff < 1)
    //     panic("pop_off");
    // c->noff -= 1;
    // if(c->noff == 0 && c->intena)
    irq_enable();
}