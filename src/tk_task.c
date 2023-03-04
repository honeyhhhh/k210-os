#include "include/proc.h"
#include "include/mm.h"
#include "include/stdio.h"
#include "include/stddef.h"
#include "include/exception.h"
#include "include/fs.h"



struct TaskControlBlock *tcb_new(uint8_t *elf_data, uint64_t datasize, struct dirent *cwd)
{
    struct TaskControlBlock *task_control_block = (struct TaskControlBlock *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct TaskControlBlock));
    initlock(&task_control_block->tcb_lock, "tcb");
    //acquire(&task_control_block->tcb_lock);

    uint64_t user_sp;
    uint64_t entry_point;
    struct MemorySet *memory_set = from_elf(elf_data, datasize,  &user_sp, &entry_point);

	PhysPageNum trap_cx_ppn = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)TRAP_CONTEXT))));
    //printk("trap_cx_ppn :[%p]\n", trap_cx_ppn);

    // pid 与　内核栈
    pid_t pid = pid_alloc();
    ks_new(pid);
	uint64_t kernel_stack_top = ks_get_top(pid);
    // 任务切换上下文
	struct TaskContext task_cx;
	memset(&task_cx, 0, sizeof(task_cx));
	task_cx.ra = (uint64_t)&e_return;
	void *dst = (void *)(kernel_stack_top - sizeof(task_cx));
	memmove(dst, &task_cx, sizeof(task_cx));

    list_init(&task_control_block->child);
    task_control_block->pid = pid;
    task_control_block->memory_set = memory_set;
    task_control_block->base_size = user_sp + PAGE_SIZE;
    task_control_block->trap_cx_ppn = trap_cx_ppn;
    task_control_block->task_cx_ptr = dst;
    task_control_block->task_status = Ready;
    task_control_block->parent = &INITPROC;
    task_control_block->cwd = edup(cwd);//&root;  //错误
    task_control_block->exit_code = 0;
    task_control_block->stimes = 0;
    task_control_block->utimes = 0;

    // ?
    for (int i = 0; i < 3; i++)
	{
		task_control_block->ofile[i] = filedup(&ftable.file[i]);	
	}
    for (int i = 3; i < NOFILE; i++)
		task_control_block->ofile[i] = NULL;


    struct context trap_cx;
    app_context_init(&trap_cx, entry_point, user_sp, token(&KERNEL_SPACE.page_table), kernel_stack_top , (uintptr_t)&e_dispatch);
    void *trap_cx_addr = (void *)ppn2pa(trap_cx_ppn);
    memmove(trap_cx_addr, &trap_cx, sizeof(trap_cx));

    //printk("[%p]\n",((struct context *)trap_cx_addr)->gr.sp);
    



    //release(&task_control_block->tcb_lock);
    return task_control_block;
}

// 当应用调用 sys_exit 系统调用主动退出或者出错由内核终止之后，会在内核中调用 exit_current_and_run_next 函数退出当前任务并切换到下一个。使用方法如下：
void exit_current_and_run_next(int exit_code)
{
    struct TaskControlBlock *task = mytask();

    if (task == &INITPROC)
        panic("initproc exit\n");
    
    //printk("task exit [%d]\n", task->pid);
    acquire(&task->tcb_lock);

    task->task_status = Zombie;  //这样它后续才能被父进程在 waitpid 系统调用的时候回收；
    task->exit_code = exit_code;  // 后续父进程在 waitpid 的时候可以收集

    // 将当前进程的所有子进程挂在初始进程 initproc 下面，其做法是遍历每个子进程，修改其父进程为初始进程，并加入初始进程的孩子向量中。
    acquire(&INITPROC.tcb_lock);
    //printk("task exit s [%d]\n", task->pid);


    struct TaskControlBlock *c =NULL;
    while( !list_empty(&task->child) )
    {
        c = elem2entry(struct TaskControlBlock, ctag, list_pop(&task->child));
        c->parent = &INITPROC;

        list_append(&INITPROC.child, &c->ctag);
    }

    release(&INITPROC.tcb_lock);

    // 当前进程占用的资源进行早期回收
    // 将地址空间中的逻辑段列表 areas 清空,将应用地址空间的所有数据被存放在的物理页帧被回收unmap，而用来存放页表的那些物理页帧此时则不会被回收。
    ms_ma_free(task->memory_set);

    // printk("pid : [%d] exit\n kheap :[%d]\n", task->pid, buddy_remain_size(HEAP_ALLOCATOR));
	// printk("frame :%d\n", frame_remain_size(&FRAME_ALLOCATOR));

    //关文件 bug
    for(int fd = 3; fd < NOFILE; fd++)
    {
        if(task->ofile[fd])
        {
            struct file *f = task->ofile[fd];
            fileclose(f);
            task->ofile[fd] = NULL;
        }
    }
    eput(task->cwd);

    struct TaskControlBlock *original_parent = task->parent;
    // 将自己的父进程设为INIT ?

    release(&task->tcb_lock);


    // 唤醒原来的父进程
    // acquire(&original_parent->tcb_lock);

    wakeup_fromsv(original_parent);

    // release(&original_parent->tcb_lock);


    // 调用 schedule 触发调度及任务切换，由于我们再也不会回到该进程的执行过程中，因此无需关心任务上下文的保存。
    uint64_t unused = 0;
    schedule((uint64_t)&unused);

}

// 暂停当前任务并切换到下一个任务，当应用调用 sys_yield 主动交出使用权、本轮时间片用尽或者由于某些原因内核中的处理无法继续的时候，就会在内核中调用此函数触发调度机制并进行任务切换。
void suspend_current_and_run_next()
{
    struct TaskControlBlock *task = mytask();
    acquire(&task->tcb_lock);

    task->task_status = Ready;
    uint64_t task_cx_ptr2 = (uint64_t)&task->task_cx_ptr;

    release(&task->tcb_lock);

    task_add(task);



    schedule(task_cx_ptr2);

}


struct TaskControlBlock *fork(struct TaskControlBlock *parent)
{
    acquire(&parent->tcb_lock);

    // printk("sizeof tcb [%d]\n", sizeof(struct TaskControlBlock));
    //printk("[%p]\n", parent->memory_set);

    struct TaskControlBlock *ttcb = (struct TaskControlBlock *)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct TaskControlBlock));
    initlock(&ttcb->tcb_lock, "ctcb");


    // printk("tcb :[%p]\n", ttcb);



    struct MemorySet *ms = from_existed_user(parent->memory_set);

    // trap上下文一致
	PhysPageNum trap_cx_ppn = pte_get_ppn(*(find_pte(&ms->page_table, va2vpn((VirtAddr)TRAP_CONTEXT))));


    // pid 与　内核栈
    pid_t pid = pid_alloc();
    ks_new(pid);
	uint64_t kernel_stack_top = ks_get_top(pid);
    // 任务切换上下文
	struct TaskContext task_cx;
	memset(&task_cx, 0, sizeof(task_cx));
	task_cx.ra = (uint64_t)&e_return;
	void *dst = (void *)(kernel_stack_top - sizeof(task_cx));
	memmove(dst, &task_cx, sizeof(task_cx));



    // tcb 赋值
    ttcb->pid = pid;
    ttcb->trap_cx_ppn = trap_cx_ppn;
    ttcb->base_size = parent->base_size;
    ttcb->task_status = Ready;
    ttcb->task_cx_ptr = dst;
    ttcb->memory_set = ms;
    ttcb->parent = parent;
    list_init(&ttcb->child);
    ttcb->cwd = edup(parent->cwd); //initproc ??
    ttcb->exit_code = 0;

    for (int i = 0; i < NOFILE; i++)
	{
        if (parent->ofile[i] != NULL)
		    ttcb->ofile[i] = filedup(parent->ofile[i]);	
	}

    // add child
    list_append(&parent->child, &ttcb->ctag);


    // 修改trap 中的内核栈
    struct context *trap_cx = (void *)ppn2pa(trap_cx_ppn);
    trap_cx->kernel_sp = kernel_stack_top;




    release(&parent->tcb_lock);
    return ttcb;

    //子进程的地址空间不是通过解析 ELF 而是通过调用 MemorySet::from_existed_user 复制父进程地址空间得到的
    //在复制地址空间的时候，子进程的 Trap 上下文也是完全从父进程复制过来的，
    // 这可以保证子进程进入用户态和其父进程回到用户态的那一瞬间 CPU 的状态是完全相同的（后面我们会让它们有一点不同从而区分两个进程）。
}


// pid==-1 等待任意子进程 pid>0 等待进程ID与pid相等的子进程 不考虑pid==0 等待组ID等于调用进程组ID的任意子进程 pid<-1 等待组ID等于pid绝对值的任意子进程
// option :WNOHANG 不阻塞　, 0阻塞(睡眠)

//1. 当正常返回的时候，waitpid返回收集到的子进程的进程ID；
//2. 如果设置了选项WNOHANG，而调用中waitpid发现没有已退出的子进程可收集，则返回0；
//3. 如果调用中出错，则返回-1，这时errno会被设置成相应的值以指示错误所在；
pid_t waitpid(pid_t pid, uint64_t code_exit_addr, int options)
{
    struct TaskControlBlock *p = mytask();
    bool flag = 0;
    struct list_elem* elem = NULL;
    struct TaskControlBlock *c = NULL;
    acquire(&p->tcb_lock);  //

    while (1) // 保证回收　符合要求的进程
    {

        elem = p->child.head.next;

        while(elem != &p->child.tail) 
        {
            c = elem2entry(struct TaskControlBlock, ctag, elem);
            acquire(&c->tcb_lock);

            if (pid == -1 || pid == c->pid)
            {
                flag = 1;
                if (c->task_status == Zombie) // 没有僵尸进程就睡眠
                {
                    pid_t pid = c->pid;

                    // // addr 用户虚拟地址　赋值为子进程退出状态　　返回  可NULL
                    // printk("exitcode [%d] [%d]\n", c->pid ,c->exit_code);
                    c->exit_code = c->exit_code << 8;
                    if (code_exit_addr != 0 && copyout2(code_exit_addr, (char *)&c->exit_code, sizeof(c->exit_code)) < 0)
                    {
                        release(&p->tcb_lock);
                        release(&c->tcb_lock);
                        return -1;
                    }
                    // 这里　移除
                    list_remove(elem);
                    // 这里回收内核栈?和它的 PID 还有它的应用地址空间存放页表的那些物理页帧..

                    release(&p->tcb_lock);
                    release(&c->tcb_lock);
                    return pid;
                }
            }

            release(&c->tcb_lock);
            elem = elem->next;
        }


        if ( !flag || p->task_status == Zombie)
        {
            release(&p->tcb_lock);
            return -1;
        }

        if (options == 0)
            sleep(&TASK_MANAGER.sleep_vec, &p->tcb_lock); // 带锁进去?
        else
            break;
    }
    return 0;
}

pid_t wait(uint64_t code_exit_addr)
{
    return waitpid(-1, code_exit_addr, 0);
}


void getTimes(struct tms *tm)
{
    struct TaskControlBlock *p = mytask();
    tm->tms_utime = p->utimes;
    tm->tms_stime = p->stimes;


    tm->tms_cutime = 0;
    tm->tms_cstime = 0;

}




void execve(char *path, char **argv, char **envp)
{
    struct TaskControlBlock *current = mytask();
    struct dirent *e = ename(path);
    uint64_t elf_date = (uint64_t)buddy_alloc(HEAP_ALLOCATOR, e->file_size); //64k
    eread(e, 0, elf_date, 0, e->file_size);

    uint64_t user_sp;
    uint64_t entry_point;
    struct MemorySet *memory_set = from_elf((uint8_t *)elf_date, e->file_size,  &user_sp, &entry_point);
	PhysPageNum trap_cx_ppn = pte_get_ppn(*(find_pte(&memory_set->page_table, va2vpn((VirtAddr)TRAP_CONTEXT))));





    acquire(&current->tcb_lock);
    // 这里需要手动回收原来的地址空间
    current->memory_set = memory_set;
    current->trap_cx_ppn = trap_cx_ppn;

    struct context trap_cx;
    app_context_init(&trap_cx, entry_point, user_sp, token(&KERNEL_SPACE.page_table), ks_get_top(current->pid) , (uintptr_t)&e_dispatch);
    void *trap_cx_addr = (void *)ppn2pa(trap_cx_ppn);
    memmove(trap_cx_addr, &trap_cx, sizeof(trap_cx));

    release(&current->tcb_lock);


    buddy_free(HEAP_ALLOCATOR, (void *)elf_date);

}