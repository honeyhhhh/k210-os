#include "include/syscall.h"
#include "include/stdio.h"
#include "include/assert.h"


/*
// 转移表，调用编号为数组下标，对应函数指针
static int (*syscalls[])(uint64_t arg[]) = {
     
};

*/



void syscall_handler(struct context *f)
{
    panic("ecall from user!\n");
    f->epc += 4;
}




