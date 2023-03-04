#include "include/sem.h"
#include "include/spinlock.h"
#include "include/proc.h"



void sem_init(struct semaphore *s, uint value, char *name) 
{
    s->value = value;  // 0
    initlock(&s->lock, "semaphore_lock");
    s->name = name;
    list_init(&s->waiters);
    //end = start = 0;
}

void sem_wait(struct semaphore *s) 
{
    acquire(&s->lock);
    s->value--;    // 1->0   0->-1
    while (s->value < 0)  // 表示资源已经被别人持有，不同于以前陷入无限循环，把当前进程加入该资源的等待队列，然后睡眠
    {
        // 总体思路是希望 sleep 将当前进程转化为 SLEEPING 状态并调用 sched 以释放 CPU，而 wakeup 则寻找一个睡眠状态的进程并把它标记为 RUNNABLE。
        // sleep(chan) , 让调用进程　在任意的 chan （称之为等待队列（wait channel），本质是链表）上休眠，释放所占 CPU。
        // wakeup(chan) 则唤醒在 chan 上休眠的所有进程，让他们的 sleep 调用返回。
        // 1、遗失的唤醒（解决方法：引入s->lock）：wait在调用sleep之前（譬如这时处理器突然收到一个中断然后开始执行中断处理，延迟了对 sleep 的调用）
        // wakeup 在　sleep　之前被调用，发现没有进程正在休眠，什么也没做，而sleep执行, 而下一个signal又在等，陷入死锁
        // 这个问题的根源在于没有维持好一个固定状态

        // 2、当 wait 带着锁 s->lock 进入睡眠后，signal就会在希望获得锁时一直阻塞，死锁。
        // 解决方法：sleep 必须将锁作为一个参数，然后在进入睡眠状态后释放之；这样能避免遗失的唤醒。一旦进程被唤醒了，sleep 在返回之前还需要重新获得锁。
        //asm volatile("ebreak");

        sleep(&s->waiters, &s->lock);
    }
    if (mytask() != NULL)
        s->pid = mytask()->pid;
    release(&s->lock);
}

void sem_signal(struct semaphore *s) 
{
    acquire(&s->lock);
    s->value++;
    if (s->value <= 0) 
    {
        // wakeup(s->queue[s->start]);
        // s->queue[s->start] = 0;
        // s->start = (s->start + 1) % NPROC;
        wakeup_fromwq(&s->waiters);
    }
    s->pid = 0;
    release(&s->lock);
}


bool sem_hold(struct semaphore *s)
{
    bool i = 1;
    acquire(&s->lock);

    //printk("[%p]\n", mytask());
    if (mytask() != NULL)
        i = (s->value <= 0 && mytask()->pid == s->pid);
    else
        i = (s->value <= 0);
    release(&s->lock);

    return i;
}


