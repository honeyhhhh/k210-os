#include "include/proc.h"
#include "include/stddef.h"
#include "include/string.h"
#include "include/riscv_asm.h"



struct Processor cpus[NCPU];
extern struct TaskControlBlock *INITPROC;




uint8_t initcode[] = 
{
	0x17, 0x05, 0x00, 0x00, 	// auipc a0, 0x0 
	0x13, 0x05, 0x45, 0x02,
	0x97, 0x05, 0x00, 0x00, 
	0x93, 0x85, 0x35, 0x02,
	0x93, 0x08, 0x70, 0x00, 
	0x73, 0x00, 0x00, 0x00,
	0x93, 0x08, 0x20, 0x00, 
	0x73, 0x00, 0x00, 0x00,		// ecall 
	0xef, 0xf0, 0x9f, 0xff, 
	0x2f, 0x69, 0x6e, 0x69,
	0x74, 0x00, 0x00, 0x24, 
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};



void procinit(void)
{
  //struct proc *p;
  
    initlock(&pid_lock, "nextpid");
//   for(p = proc; p < &proc[NPROC]; p++) {
//       initlock(&p->lock, "proc");

      // Allocate a page for the process's kernel stack.
      // Map it high in memory, followed by an invalid
      // guard page.
      // char *pa = kalloc();
      // // printf("[procinit]kernel stack: %p\n", (uint64)pa);
      // if(pa == 0)
      //   panic("kalloc");
      // uint64 va = KSTACK((int) (p - proc));
      // // printf("[procinit]kvmmap va %p to pa %p\n", va, (uint64)pa);
      // kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      // p->kstack = va;
//   }
  //kvminithart();

    memset(cpus, 0, sizeof(cpus));
}


// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
uint64_t cpuid()
{
    uint64_t id = r_tp();
    return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct Processor* mycpu()
{
    uint64_t id = cpuid();
    struct Processor *c = &cpus[id];
    return c;
}

struct TaskControlBlock *mytask()
{
    push_off();
    struct Processor *c = mycpu();
    struct TaskControlBlock *p = c->current;
    pop_off();
    return p;
}


void sleep(struct list *wait_queue, struct spinlock *lock)
{
    struct TaskControlBlock *p = mytask();
	if (lock != NULL)
	{
		acquire(&p->tcb_lock);
		release(lock);
	}

	// p　不可能在　ready_queue链表中　，　假设也不在　wait_queue　链表中
	if (p->tag.next != NULL || p->tag.prev != NULL)
		panic("????");
	list_append(wait_queue, &p->tag); 
	p->task_status = Sleeping;

	// 进入睡眠，让出CPU资源


	// 被唤醒
	// list_remove(&p->tag); 在wakeup中已经pop

	if (lock != NULL)
	{
		release(&p->tcb_lock);
		acquire(lock);
	}
}

void wakeup_fromwq(struct list *wait_queue)
{
	if (list_empty(wait_queue))
		panic("wait list empty\n");
	struct TaskControlBlock *p = elem2entry(struct TaskControlBlock, tag, list_pop(wait_queue));
	acquire(&p->tcb_lock);
	if (p->task_status != Sleeping)
		panic("!!!!\n");
	p->task_status = Ready;
	release(&p->tcb_lock);
}







