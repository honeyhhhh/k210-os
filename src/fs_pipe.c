#include "include/stddef.h"
#include "include/proc.h"
#include "include/mm.h"
#include "include/string.h"
#include "include/pipe.h"
#include "include/fs.h"
#include "include/file.h"




int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == NULL || (*f1 = filealloc()) == NULL)
    goto bad;
  if((pi = (struct pipe*)buddy_alloc(HEAP_ALLOCATOR, sizeof(struct pipe))) == NULL)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe");
  list_init(&pi->wwaiter);
  list_init(&pi->rwaiter);
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    buddy_free(HEAP_ALLOCATOR,pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
    wakeup_fromwq(&pi->rwaiter);
  } else {
    pi->readopen = 0;
    wakeup_fromwq(&pi->wwaiter);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    buddy_free(HEAP_ALLOCATOR,pi);
  } else
    release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64_t addr, int n)
{
  int i;
  char ch;
  struct TaskControlBlock *pr = mytask();
    //printk("p write [%d]\n", pr->pid);
  acquire(&pi->lock);
  for(i = 0; i < n; i++){
    while(pi->nwrite == (pi->nread + PIPESIZE)){  //DOC: pipewrite-full
      if(pi->readopen == 0 || pr->task_status == Zombie){
        release(&pi->lock);
        return -1;
      }
      wakeup_fromwq(&pi->rwaiter);
      sleep(&pi->wwaiter, &pi->lock);
    }
    // if(copyin(pr->pagetable, &ch, addr + i, 1) == -1)
    if(copyin2(&ch, addr + i, 1) == -1)
      break;
    pi->data[pi->nwrite++ % PIPESIZE] = ch;
  }
  wakeup_fromwq(&pi->rwaiter);
  release(&pi->lock);
  return i;
}

int
piperead(struct pipe *pi, uint64_t addr, int n)
{
  int i;
  struct TaskControlBlock *pr = mytask();
  char ch;
    //printk("p read [%d]\n", pr->pid);

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(pr->task_status == Zombie){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->rwaiter, &pi->lock); //DOC: piperead-sleep
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(pi->nread == pi->nwrite)
      break;
    ch = pi->data[pi->nread++ % PIPESIZE];
    // if(copyout(pr->pagetable, addr + i, &ch, 1) == -1)
    if(copyout2(addr + i, &ch, 1) == -1)
      break;
  }
  wakeup_fromwq(&pi->wwaiter);  //DOC: piperead-wakeup
  release(&pi->lock);
  return i;
}
