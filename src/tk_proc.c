#include "include/proc.h"
#include "include/stddef.h"
#include "include/string.h"
#include "include/riscv_asm.h"
#include "include/exception.h"
#include "include/sleep.h"


struct Processor cpus[NCPU];
struct TaskControlBlock INITPROC;
uint8_t initcode1[] = 
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

	0x93, 0x08, 0xC0, 0x07,		// li a7, 124
	0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x1f, 0xff,     // jal ra, <start>
    //<loop>
    //0xef, 0x00, 0x00, 0x00,     // jal ra, <loop>
};

uint8_t initcode[] = {
	0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 
	0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 
	0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 
	0x13, 0x00, 0x00, 0x00, 
	
	0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0x80, 0x07, 0x63, 0x12, 0x05, 0x04, 
	0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x05, 0x0F, 0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x05, 0x0A, 
	0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0x00, 0x05, 0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0x40, 0x05, 
	0x63, 0x18, 0x05, 0x02, 0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x45, 0x0E, 0x17, 0x05, 0x00, 0x00, 
	0x13, 0x05, 0xC5, 0x07, 0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0xC0, 0x02, 0x6F, 0x00, 0x00, 0x00, 
	0x13, 0x05, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0x40, 0x03, 0x6F, 0xF0, 0xDF, 0xFC, 
	0x13, 0x05, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0xE7, 0x80, 0x40, 0x02, 0x6F, 0xF0, 0x1F, 0xFE, 
	0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00, 0x67, 0x80, 0x00, 0x00, 0x93, 0x08, 0x10, 0x00, 
	0x73, 0x00, 0x00, 0x00, 0x67, 0x80, 0x00, 0x00, 0x93, 0x08, 0x30, 0x00, 0x73, 0x00, 0x00, 0x00, 
	0x67, 0x80, 0x00, 0x00, 0x93, 0x08, 0x80, 0x03, 0x73, 0x00, 0x00, 0x00, 0x67, 0x80, 0x00, 0x00, 
	0x93, 0x08, 0x70, 0x01, 0x73, 0x00, 0x00, 0x00, 0x67, 0x80, 0x00, 0x00, 0x93, 0x08, 0xD0, 0x05, 
	0x73, 0x00, 0x00, 0x00, 0x67, 0x80, 0x00,
	0x00, 0x62, 0x75, 0x73, 0x79, 0x62, 0x6F, 0x78, // busybox
	0x00, 0x2F, 0x64, 0x65, 0x76, 0x2F, 0x63, 0x6F, 0x6E, 0x73, 0x6F, 0x6C, 0x65, 0x00, 0x00, 0x00, // /dev/console
	0x00, 0x73, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, // sh
	0x00, 0x6C, 0x75, 0x61, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x63, 0x6F, 0x64, 0x65, 0x2E, 0x73, 0x68, // lua_testcode.sh
	0x00, 0x62, 0x75, 0x73, 0x79, 0x62, 0x6F, 0x78, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x63, 0x6F, 0x64, 0x65, 0x2E, 0x73, 0x68, //busybox_testcode.sh
	0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

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
    app_context_init(&trap_cx, 0, user_stack_top, token(&KERNEL_SPACE.page_table), kernel_stack_top , (uintptr_t)&e_dispatch);

	ms_map_uinit(memory_set, initcode, sizeof(initcode), &trap_cx, sizeof(trap_cx));

	PhysPageNum initpos = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)0))));
	printk("u :initcode:0 -> [%p]\n", initpos);
	printk("incode [%p]\n", initcode);
	uint64_t *o = (void *)ppn2pa(initpos);
	printk("[%p]\n", *o);

	INITPROC.memory_set = memory_set;
	

	PhysPageNum trap_cx_ppn = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)TRAP_CONTEXT))));
	// warn("u :trap_cx_ppn:[%p] -> [%p]\n", TRAP_CONTEXT,trap_cx_ppn);
	INITPROC.trap_cx_ppn = trap_cx_ppn;
	// struct context *t = (void *)ppn2pa(INITPROC.trap_cx_ppn);
	// printk("v epc [%p]\n", t->epc);
	// for (int i = 0; i < 36; i++)
	// 	printk("[%d] :[%p]\n", i, ((uint64_t *)t)[i]);
	// printk("v u sp [%p]\n", t->gr.sp);


	INITPROC.base_size = user_stack_top;

	INITPROC.task_cx_ptr = dst;
	// struct TaskContext *l = INITPROC.task_cx_ptr;
	// printk("ra:[%p]\n", l->ra);
	// printk("kernel stack top [%p]\n", kernel_stack_top);
	// printk("task cs size [%p]\n", sizeof(task_cx));

	INITPROC.exit_code = 0;

	INITPROC.parent = NULL;

	INITPROC.task_status = Ready;
	list_init(&INITPROC.child);
	
    // ?
    for (int i = 0; i < 3; i++)
	{
		INITPROC.ofile[i] = filedup(&ftable.file[i]);
	}
	for (int i = 3; i < NOFILE; i++)
		INITPROC.ofile[i] = NULL;

	INITPROC.cwd = NULL;

	// INITPROC.tag.next = INITPROC.tag.prev = INITPROC.all.prev = INITPROC.all.next = NULL;
	release(&INITPROC.tcb_lock);

	// 将任务添加到　队列
	task_add(&INITPROC);

	// ?
	//mycpu()->current = &INITPROC;

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
	//printk("satp [%p]\n",s);
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
	//printk("wan get lock [%d]\n",p->pid);
	if (lock != &p->tcb_lock) //防止 子进程唤不醒父进程 死锁
	{

		acquire(&p->tcb_lock);
		//printk("get  lock [%d] \n",p->pid);

		release(lock);
	}

	// p　不可能在　ready_queue链表中　，　假设也不在　wait_queue　链表中
	if (p->tag.next != NULL || p->tag.prev != NULL)
		panic("????");

    uint64_t task_cx_ptr2 = (uint64_t)&p->task_cx_ptr;
	list_append(wait_queue, &p->tag); 
	p->task_status = Sleeping;

	// 进入睡眠，让出CPU资源
	schedule(task_cx_ptr2);
	

	// 被唤醒
	// list_remove(&p->tag); 在wakeup中已经pop
	//printk("wake up [%d]",p->pid);

	if (lock != &p->tcb_lock)
	{
		release(&p->tcb_lock);
		acquire(lock);
	}
}

void wakeup_fromwq(struct list *wait_queue)
{
	//printk("[%d] come to wakeup someone\n", mytask()->pid);
	if (list_empty(wait_queue))
		return ;
	struct TaskControlBlock *p = elem2entry(struct TaskControlBlock, tag, list_pop(wait_queue));
	//acquire(&p->tcb_lock);
	if (p->task_status != Sleeping)
		panic("!!!!\n");
	p->task_status = Ready;
	task_add(p);
	//release(&p->tcb_lock);
}

// Wake up p if it is sleeping in wait(); used by exit().
// Caller must hold p->lock.
void wakeup_fromsv(struct TaskControlBlock *p)
{
	if (elem_find(&TASK_MANAGER.sleep_vec, &p->tag))
	{
		p->task_status = Ready;
		list_remove(&p->tag);
		task_add(p);
	}
	// else
	// {
	// 	printk("no found someone wait you!\n");
	// }
}


static void run(struct Processor *cpu)
{
	while (1)
	{
		
		//msleep(1);
		struct TaskControlBlock *task = task_fetch();

		if (list_empty(&TASK_MANAGER.ready_queue))
		{
			if (!list_empty(&TASK_MANAGER.all_task))
			{
				struct TaskControlBlock *t = elem2entry(struct TaskControlBlock , all, list_pop(&TASK_MANAGER.all_task));
   				list_push(&TASK_MANAGER.ready_queue, &t->tag);
			}
		}

		if (task != NULL)
		{
			//printk("irq [%d]\n",irq_get());
			//irq_enable();

			//printk("fetch [%d]\n", task->pid);
			//acquire(&task->tcb_lock);
			task->task_status = Running;
			uint64_t next_task_cx_ptr = (uint64_t)&task->task_cx_ptr; // kernel_stack_top - sizeof(task_cx);
			//release(&task->tcb_lock);

			cpu->current = task;


			__switch((uint64_t)&cpu->idle_task_cx, next_task_cx_ptr);

		}
	}
	
}

void task_run()
{
	struct Processor *p = mycpu();
	run(p);
}


void schedule(const uint64_t switched_task_cx_ptr2)
{
	struct Processor *cpu = mycpu();
	uint64_t idle_task_cx_ptr2 = (uint64_t)&cpu->idle_task_cx;

	//printk("from %p, to  %p\n",switched_task_cx_ptr2, idle_task_cx_ptr2);

	__switch(switched_task_cx_ptr2, idle_task_cx_ptr2);
}






/* ---------内核　如何从　用户地址空间获取　系统调用参数----------- */




// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64_t dst, void *src, uint64_t len)
{
	// struct proc *p = myproc();

	if(user_dst){
		// return copyout(p->pagetable, dst, src, len);
		return copyout2(dst, src, len);
	} else {

		memmove((void *)dst, src, len);
		return 0;
	}
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64_t src, uint64_t len)
{
  // struct proc *p = myproc();
	if(user_src){
		// return copyin(p->pagetable, dst, src, len);
		return copyin2(dst, src, len);
	} else {
		memmove(dst, (char*)src, len);
		return 0;
	}
}




