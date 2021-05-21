#ifndef _SEM_H
#define _SEM_H


#include "spinlock.h"
#include "list.h"
#include "proc.h"


struct semaphore 
{
    uint value;
    struct spinlock lock;
    // struct proc *queue[NPROC];
    // int end;
    // int start;

    struct list waiters;

    pid_t pid;
    char *name;
};

void sem_init(struct semaphore *s, uint value, char *name);
void sem_wait(struct semaphore *s);
void sem_signal(struct semaphore *s);
void sem_broadcast(struct semaphore *s);

#endif



