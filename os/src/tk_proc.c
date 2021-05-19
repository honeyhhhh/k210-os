#include "include/proc.h"
#include "include/stddef.h"
#include "include/string.h"
#include "include/riscv_asm.h"



struct Processor cpus[NCPU];

int nextpid = 1;
struct spinlock pid_lock;

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