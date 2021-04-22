#include "include/syscall.h"
#include "include/stdio.h"



void syscall_handler(struct context *f)
{
    f->epc += 4;


    


}




