#include "stddef.h"
#include "proc.h"


struct TaskManager TASK_MANAGER;




void tm_init()
{
    initlock(&TASK_MANAGER.lock);
    list_init(&TASK_MANAGER.ready_queue);
    list_init(&TASK_MANAGER.all_task);
}


void task_add(struct TaskControlBlock *task)
{
    list_append(&TASK_MANAGER.ready_queue, &task->tag);
    list_append(&TASK_MANAGER.all_task, &task->all);
}
struct TaskControlBlock *task_fetch()
{
    struct TaskControlBlock *p = elem2entry(struct TaskContolBlock, tag, list_pop(&TASK_MANAGER.ready_queue));
    return p;
}