#include "include/proc.h"
#include "include/stddef.h"
#include "include/string.h"
#include "include/riscv_asm.h"
#include "include/exception.h"



struct Processor cpus[NCPU];
struct TaskControlBlock INITPROC;
uint8_t initcode[] = 
{

	// 0x17, 0x05, 0x00, 0x00, 	// auipc a0, 0x0
    //<loop>
    //0xef, 0x00, 0x00, 0x00,     // jal ra, <loop>
	// 0x13, 0x05, 0x45, 0x02,
	// 0x97, 0x05, 0x00, 0x00, 
	// 0x93, 0x85, 0x35, 0x02,
	// 0x93, 0x08, 0x70, 0x00, 
	// 0x73, 0x00, 0x00, 0x00,
	// 0x93, 0x08, 0x20, 0x00, 
	// 0x73, 0x00, 0x00, 0x00,		// ecall 
	// 0xef, 0xf0, 0x9f, 0xff, 
	// 0x2f, 0x69, 0x6e, 0x69,
	// 0x74, 0x00, 0x00, 0x24, 
	// 0x00, 0x00, 0x00, 0x00,
	// 0x00, 0x00, 0x00, 0x00

    0x13, 0x00, 0x00, 0x00,     // nop
    //<start>
    0x17, 0x05, 0x00, 0x00,     // auipc a0, 0x0 
    0x13, 0x05, 0x05, 0x00,     // mv a0, a0 
    0x93, 0x08, 0x60, 0x01,     // li a7, 22 
    0x73, 0x00, 0x00, 0x00,     // ecall 
    0xef, 0xf0, 0x1f, 0xff,     // jal ra, <start>
    //<loop>
    0xef, 0x00, 0x00, 0x00,     // jal ra, <loop>
};



void procinit(void)
{
	pid_init();	//初始化pid分配器
	tm_init(); //初始化任务管理器

    memset(cpus, 0, sizeof(cpus)); //初始化处理器管理结构
}

void add_initproc()
{
	//warn("INITPROC TCB addr : [%p]\n", &INITPROC);
	//warn("[%p]\n",&INITPROC.task_cx_ptr);
	//struct TaskControlBlock *p = (struct TaskControlBlock *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct TaskControlBlock));
	initlock(&INITPROC.tcb_lock, "inittcb"); // 锁
	acquire(&INITPROC.tcb_lock);
	INITPROC.pid = pid_alloc();			// pid


	// 生成用户地址空间，并初始化用户页表
	struct MemorySet *memory_set = (struct MemorySet *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct MemorySet ));
	// map　跳板trampoline(vs = TRAMPOLINE, ps = strampoline, size = PAGE_SIZE)
	// map　Trap上下文 (vs = TRAP_CONTEXT, ps = framealloc, size = PAGE_SIZE)
	// map  initcode(vs = 0, ps = initcode, size = sizeof(initcode)向上取整)  
	// map 　user_stack(vs = 0+sizeof(initcode)向上取整+4K, ps = framealloc, size = USER_STACK_SIZE)

	// 生成内核栈，并将任务切换的上下文写在内核栈顶
	ks_new(INITPROC.pid);
	uint64_t kernel_stack_top = ks_get_top(INITPROC.pid);
	struct TaskContext task_cx;
	memset(&task_cx, 0, sizeof(task_cx));
	task_cx.ra = (uint64_t)&e_return;
	void *dst = (void *)(kernel_stack_top - sizeof(task_cx));
	memmove(dst, &task_cx, sizeof(task_cx));

	// 生成Trap上下文，准备写入次高页面
	struct context trap_cx;
	uint64_t user_stack_top = vpn2va(va_ceil((VirtAddr)sizeof(initcode))) + PAGE_SIZE + USER_STACK_SIZE;
    app_context_init(&trap_cx, 0, user_stack_top, token(&KERNEL_SPACE.page_table),kernel_stack_top , (uintptr_t)&e_dispatch);

	ms_map_uinit(memory_set, initcode, sizeof(initcode), &trap_cx, sizeof(trap_cx));

	// PhysPageNum initpos = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)0))));
	// warn("u :initcode:0 -> [%p]\n", initpos);
	// warn("incode [%p]\n", initcode);
	// uint64_t *o = (void *)ppn2pa(initpos);
	// printf("[%p]\n", *o);

	INITPROC.memory_set = memory_set;
	

	PhysPageNum trap_cx_ppn = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)TRAP_CONTEXT))));
	// warn("u :trap_cx_ppn:[%p] -> [%p]\n", TRAP_CONTEXT,trap_cx_ppn);
	INITPROC.trap_cx_ppn = trap_cx_ppn;
	// struct context *t = (void *)ppn2pa(INITPROC.trap_cx_ppn);
	// printf("v epc [%p]\n", t->epc);
	// for (int i = 0; i < 36; i++)
	// 	printf("[%d] :[%p]\n", i, ((uint64_t *)t)[i]);
	// printf("v u sp [%p]\n", t->gr.sp);


	INITPROC.base_size = user_stack_top;

	INITPROC.task_cx_ptr = dst;
	// struct TaskContext *l = INITPROC.task_cx_ptr;
	// printf("ra:[%p]\n", l->ra);
	// printf("kernel stack top [%p]\n", kernel_stack_top);
	// printf("task cs size [%p]\n", sizeof(task_cx));

	INITPROC.exit_code = 0;

	INITPROC.parent = NULL;

	INITPROC.task_status = Ready;

	// INITPROC.tag.next = INITPROC.tag.prev = INITPROC.all.prev = INITPROC.all.next = NULL;
	release(&INITPROC.tcb_lock);

	// 将任务添加到　队列
	task_add(&INITPROC);

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



uint64_t mytask_token()
{
	struct TaskControlBlock *p = mytask();
	uint64_t s = 0;
	if (p == NULL)
		s =  token(&KERNEL_SPACE.page_table);
	else
		s = token(&p->memory_set->page_table);
	//printf("satp [%p]\n",s);
	return s;
}

struct context *mytask_trap_cx()
{
	struct TaskControlBlock *p = mytask();
	struct context *s = NULL;
	if (p == NULL)
		s = (struct context *)TRAP_CONTEXT;
	else
		s = (void *)ppn2pa(p->trap_cx_ppn);
	return s;	
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
	printf("pid [%p]\n",p->pid);
	acquire(&p->tcb_lock);
	if (p->task_status != Sleeping)
		panic("!!!!\n");
	p->task_status = Ready;
	release(&p->tcb_lock);
}


static void run(struct Processor *cpu)
{
	while (1)
	{
		struct TaskControlBlock *task = task_fetch();
		if (task != NULL)
		{
			acquire(&task->tcb_lock);
			task->task_status = Running;
			uint64_t next_task_cx_ptr = (uint64_t)&task->task_cx_ptr; // kernel_stack_top - sizeof(task_cx);
			release(&task->tcb_lock);

			cpu->current = task;
			// printf("into switch:[%p]\n", &__switch);
			// printf("idle tcx:[%p]\n", &cpu->idle_task_cx);
			// printf("ntask tcx:[%p]\n", next_task_cx_ptr);

			//printf("cpu idle task cx [%p]\n", &cpu->idle_task_cx);
			__switch((uint64_t)&cpu->idle_task_cx, next_task_cx_ptr);

		}
	}
	
}

void task_run()
{
	struct Processor *p = mycpu();
	run(p);
}







