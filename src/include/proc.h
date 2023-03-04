#ifndef _PROC_H
#define _PROC_H

#include "stddef.h"
#include "mm.h"
#include "spinlock.h"
#include "list.h"
#include "exception.h"
#include "fs.h"
#include "file.h"


#define SIGCHLD   17


/* 进程标识符 */
typedef uint64_t pid_t;
struct PidAllocator
{
    struct spinlock lock;
    uint64_t current;           // 初始１
    uint64_t recycled_stack[31];
    uint8_t sp;
};
extern struct PidAllocator PID_ALLOCATOR;
void pid_init();
pid_t pid_alloc();
void pid_dealloc(pid_t pid);

/*  内核栈　*/
struct Kstack
{
    pid_t pid;
};
void kernel_stack_pos(pid_t pid, uint64_t *pos);    //根据进程标识符计算内核栈在内核地址空间中的位置，随即将一个逻辑段插入内核地址空间 KERNEL_SPACE 中
void ks_new(pid_t pid);        //从一个已分配的进程标识符中对应生成一个内核栈 KernelStack
//void ks_free(struct　KStack *self);     // 在内核地址空间中将对应的逻辑段删除, 并将对应的物理页回收
uintptr_t ks_get_top(pid_t pid);






// 13 个　8B
struct TaskContext
{
    uint64_t ra;    // trap_return()
    //uint64_t sp;
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
    UNINIT, Ready, Running, Zombie, Sleeping
};

struct TaskControlBlock
{
    // immutable
    pid_t pid;
    //struct Kstack kernel_stack;


    // mutable
    struct spinlock tcb_lock;
    enum TaskStatus task_status;        // 维护当前进程的执行状态
    struct TaskContext *task_cx_ptr;              // 指出一个暂停的任务的任务上下文在内核地址空间（更确切的说是在自身内核栈）中的位置，用于任务切换。
    PhysPageNum trap_cx_ppn;            // 指出了应用地址空间中的 Trap 上下文被放在的物理页帧的物理页号。
    uintptr_t base_size;                 // 应用数据仅有可能出现在应用地址空间低于 base_size 字节的区域中
    struct MemorySet *memory_set;       // 用户进程地址空间，内存管理的信息
    int32_t exit_code;                 // 当进程调用 exit 系统调用主动退出或者执行出错由内核终止的时候，它的退出码 exit_code 会被内核保存在它的任务控制块中，并等待它的父进程通过 waitpid 回收它的资源的同时也收集它的 PID 以及退出码。
    struct TaskControlBlock *parent;  //父进程的指针, 有利于维护父进程对于子进程的一些特殊操作。使用 Weak 而非 Arc 来包裹另一个任务控制块，因此这个智能指针将不会影响父进程的引用计数。

    struct list child;
    struct list_elem ctag;

    struct list_elem tag;
    struct list_elem all;

    struct dirent *cwd;
    struct file *ofile[NOFILE];

    int stimes;  // 
    int utimes;  // 


};

/* 任务管理器 */
struct TaskManager
{
    struct spinlock lock;
    struct list ready_queue;
    struct list sleep_vec;  // for wait/waitpid
    struct list all_task;   // for 
};
extern struct TaskManager TASK_MANAGER;
void tm_init();
void task_add(struct TaskControlBlock *task);
struct TaskControlBlock *task_fetch();
void task_create(struct TaskControlBlock *task);

/* 处理器管理结构　Per-CPU  */
struct Processor
{
    struct TaskControlBlock *current;  //在当前处理器上正在执行的任务
    struct TaskContext *idle_task_cx; //表示当前处理器上的 idle 执行流的任务上下文。
    // 每个 Processor 都有一个不同的 idle 执行流，它们运行在每个核各自的启动栈上，功能是尝试从任务管理器中选出一个任务来在当前核上执行。在内核初始化完毕之后，每个核都会通过调用 run_tasks 函数来进入 idle 执行流：
    // 而反过来，当一个应用用尽了内核本轮分配给它的时间片或者它主动调用 yield 系统调用交出 CPU 使用权之后，进入内核后它会调用 schedule 函数来切换到 idle 执行流并开启新一轮的任务调度。
    int depth_of_ir;
    bool ir_enable;

};
extern struct Processor cpus[NCPU];
extern struct TaskControlBlock INITPROC;

struct TaskControlBlock *tcb_new(uint8_t *elf_data, uint64_t datasize, struct dirent *cwd);
void exit_current_and_run_next(int32_t exit_code);
void suspend_current_and_run_next();
struct TaskControlBlock *fork(struct TaskControlBlock *parent);
pid_t waitpid(pid_t pid, uint64_t code_exit_addr, int options);
pid_t wait(uint64_t code_exit_addr);
void execve(char *path, char **argv, char **envp);


void schedule(const uint64_t switched_task_cx_ptr2);

uint64_t cpuid();
struct Processor* mycpu();
struct TaskControlBlock *mytask();
uint64_t mytask_token();
struct context *mytask_trap_cx();
void getTimes(struct tms *tm);


void procinit();
void add_initproc();
void task_run();
void sleep(struct list *wait_queue, struct spinlock *lock);
void wakeup_fromwq(struct list *wait_queue);
void wakeup_fromsv(struct TaskControlBlock *p);




extern void __switch(const uint64_t current_task_cx_ptr, const uint64_t next_task_cx_ptr);


int either_copyout(int user_dst, uint64_t dst, void *src, uint64_t len);
int either_copyin(void *dst, int user_src, uint64_t src, uint64_t len);



#endif