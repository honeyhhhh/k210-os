#ifndef _PROC_H
#define _PROC_H

#include "stddef.h"
#include "mm.h"
#include "spinlock.h"


/* 进程标识符 */
typedef uint64_t pid_t;
struct PidAllocator
{
    struct spinlock pid_lock;
    uint64_t current;
    uint64_t recycled_stack[31];
    uint8_t sp;
};
extern struct PidAllocator PID_ALLOCATOR;
pid_t pid_alloc(struct PidAllocator *self);
void pid_dealloc(struct PidAllocator *self, pid_t pid);

/*  内核栈　*/
struct Kstack
{
    pid_t pid;
};
void kernel_stack_pos(pid_t pid, uint64_t *pos);    //根据进程标识符计算内核栈在内核地址空间中的位置，随即将一个逻辑段插入内核地址空间 KERNEL_SPACE 中
void ks_init(struct Kstack *self, pid_t pid);        //从一个已分配的进程标识符中对应生成一个内核栈 KernelStack
//void ks_free(struct　KStack *self);     // 在内核地址空间中将对应的逻辑段删除, 并将对应的物理页回收
uintptr_t ks_get_top(struct Kstack *self);







struct TaskContext
{
    uint64_t ra;    // trap_return()
    uint64_t sp;
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
};

/* 进程控制块　*/
enum TaskStatus
{
    Ready, Running, Zombie
};

struct TaskControlBlock
{
    struct spinlock tcb_lock;
    pid_t pid;
    struct Kstack kernel_stack;


    enum TaskStatus task_status;
    uintptr_t task_cx_ptr;
    PhysPageNum trap_cx_ppn;
    uint64_t base_size;
    struct MemorySet *memory_set;
    
};




/* 任务管理器 */
struct TaskManager
{

};
extern struct TaskManager TASK_MANAGER;


/* 处理器管理结构　Per-CPU  */
struct Processor
{

};












#endif