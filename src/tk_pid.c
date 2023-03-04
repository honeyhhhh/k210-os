#include "include/proc.h"
#include "include/assert.h"
#include "include/mm.h"

struct PidAllocator PID_ALLOCATOR;


void pid_init()
{
    initlock(&PID_ALLOCATOR.lock, "pid_alloc");
    PID_ALLOCATOR.current = 1;
    PID_ALLOCATOR.sp = 0;
}


pid_t pid_alloc()
{
    acquire(&PID_ALLOCATOR.lock);
    pid_t result = 0;
    if (PID_ALLOCATOR.sp != 0)
    {
        PID_ALLOCATOR.sp--;
        result = PID_ALLOCATOR.recycled_stack[PID_ALLOCATOR.sp];
    }
    else
    {
        PID_ALLOCATOR.current += 1;
        result = PID_ALLOCATOR.current - 1;
    }
    release(&PID_ALLOCATOR.lock);


    return result;

}

void pid_dealloc(pid_t pid)
{
    acquire(&PID_ALLOCATOR.lock);
    assert(pid < PID_ALLOCATOR.current);
    for (int i = 0; i < PID_ALLOCATOR.sp; i++)
    {
        if (PID_ALLOCATOR.recycled_stack[PID_ALLOCATOR.sp] == pid)
            panic("pid [%p] has been deallocated", pid);
    }
    PID_ALLOCATOR.recycled_stack[PID_ALLOCATOR.sp] = pid;
    PID_ALLOCATOR.sp++;
    release(&PID_ALLOCATOR.lock);
}


// TRAMPOLINE =  (ULONG_MAX - PAGE_SIZE + 1)
// TRAP_CONTEXT = TRAMPOLINE - PAGE_SIZE
// top高地址pos[1]　bottom低地址pos[0]
void kernel_stack_pos(pid_t pid, uint64_t *pos)
{
    pos[1] = TRAP_CONTEXT - pid * (KERNEL_STACK_SIZE + PAGE_SIZE); // 内核栈+空白页
    pos[0] = pos[1] - KERNEL_STACK_SIZE;
}
//一个已分配的进程标识符中对应生成一个内核栈 KernelStack 
void ks_new(pid_t pid)
{
    uint64_t pos[2];
    kernel_stack_pos(pid, pos);

    // KERNEL_SPACE锁
    insert_framed_area(&KERNEL_SPACE, (VirtAddr)pos[0], (VirtAddr)pos[1], VM_R|VM_W);
}
//void ks_free(pid_t pid);     // 在内核地址空间中将对应的逻辑段删除, 并将对应的物理页回收
uintptr_t ks_get_top(pid_t pid)
{
    return TRAP_CONTEXT - pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
}







