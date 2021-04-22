#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "stddef.h"
#include "exception.h"

/* syscall number */

/* 文件系统相关 */
#define SYS_getcwd 1//17
#define SYS_pipe2 2//59
#define SYS_dup 3//23
#define SYS_dup3 4//24
#define SYS_chdir 5//49
#define SYS_openat 6//56
#define SYS_close 7//57
#define SYS_getdents64 8//61
#define SYS_read 9//63
#define SYS_write 10//64
#define SYS_linkat 11//37
#define SYS_unlinkat 12//35
#define SYS_mkdirat 13//34
#define SYS_umount2 14//39
#define SYS_mount 15//40
#define SYS_fstat 16//80

/* 进程管理相关 */
#define SYS_clone 17//220
#define SYS_execve 18//221
#define SYS_wait4 19//260
#define SYS_exit 20//93
#define SYS_getppid 21//173
#define SYS_getpid 22//172


/* 内存管理相关 */
#define SYS_brk 23//214
#define SYS_munmap 24//215
#define SYS_mmap 25//222

/* 其他 */
#define SYS_times 26//153
#define SYS_uname 27//160
#define SYS_sched_yield 28//124
#define SYS_gettimeofday 29//169
#define SYS_nanosleep 30//101


//extern void *sys_call_table[];


void syscall_handler(struct context *f);



#endif 
