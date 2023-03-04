#include "include/proc.h"


struct TaskManager TASK_MANAGER;




void tm_init()
{
    initlock(&TASK_MANAGER.lock, "task_manage");
    list_init(&TASK_MANAGER.ready_queue);
    list_init(&TASK_MANAGER.all_task);
    list_init(&TASK_MANAGER.sleep_vec);
}

// 任务控制块经常需要被放入/取出，如果直接移动任务控制块自身将会带来大量的数据拷贝开销，而对于智能指针进行移动则没有多少开销。
//  提供 add/fetch 两个操作，前者表示将一个任务加入队尾，后者则表示从队头中取出一个任务来执行。从调度算法来看，这里用到的就是最简单的 RR 算法。
// 即使在多核情况下，我们也只有单个任务管理器共享给所有的核来使用。然而在其他设计中，每个核可能都有一个自己独立的任务管理器来管理仅可以在自己上面运行的任务。

void task_create(struct TaskControlBlock *task)
{
    acquire(&TASK_MANAGER.lock);
    list_push(&TASK_MANAGER.all_task, &task->all);
    release(&TASK_MANAGER.lock);
}


void task_add(struct TaskControlBlock *task)
{
    acquire(&TASK_MANAGER.lock);

    list_append(&TASK_MANAGER.ready_queue, &task->tag);
    //list_append(&TASK_MANAGER.all_task, &task->all);
    release(&TASK_MANAGER.lock);

}
struct TaskControlBlock *task_fetch()
{
    acquire(&TASK_MANAGER.lock);

    if (list_empty(&TASK_MANAGER.ready_queue))
    {
        release(&TASK_MANAGER.lock);
        return NULL;
    }

    struct TaskControlBlock *p = elem2entry(struct TaskControlBlock , tag, list_pop(&TASK_MANAGER.ready_queue));
    // printk("fetch :pid [%p]\n", p->pid);
    // printk("fetch :token [%p]\n", token(&p->memory_set->page_table));
    // printk("fetch :switch cx [%p]\n",p->task_cx_ptr);
    // printk("fetch :trap cx [%p]\n", p->trap_cx_ppn);
    // printk("fetch :status [%p]\n",p->task_status);
    release(&TASK_MANAGER.lock);

    return p;
}