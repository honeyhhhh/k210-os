#include "include/syscall.h"
#include "include/stdio.h"
#include "include/assert.h"
#include "include/stddef.h"
#include "include/proc.h"
#include "include/timer.h"
#include "include/sleep.h"
#include "include/sysinfo.h"
#include "include/pipe.h"
#include "include/file.h"
#include "include/fs.h"
#include "include/mm.h"



static uint64_t argraw(int n)
{
    struct context *cx = mytask_trap_cx();
    switch (n) 
    {
        case 0:
            return cx->gr.a0;
        case 1:
            return cx->gr.a1;
        case 2:
            return cx->gr.a2;
        case 3:
            return cx->gr.a3;
        case 4:
            return cx->gr.a4;
        case 5:
            return cx->gr.a6;
    }
    panic("argraw");
    return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip)
{
    *ip = argraw(n);
    return 0;
}
int argaddr(int n, uint64_t *ip)
{
    *ip = argraw(n);
    return 0;
}


static int argfd(int n, int *pfd, struct file **pf)
{
    int fd;
    struct file *f;

    if(argint(n, &fd) < 0)
        return -1;
    if(fd < 0 || fd >= NOFILE || (f = mytask()->ofile[fd]) == NULL)
        return -1;
    if(pfd)
        *pfd = fd;
    if(pf)
        *pf = f;
    return 0;
}




int fetchstr(uint64_t addr, char *buf, int max)
{
    // struct proc *p = myproc();
    // int err = copyinstr(p->pagetable, buf, addr, max);
    int err = copyinstr2(buf, addr, max);
    if(err < 0)
        return err;
    return strlen(buf);
}




int argstr(int n, char *buf, int max)
{
    uint64_t addr;
    if(argaddr(n, &addr) < 0)
        return -1;
    //printk("%p\n", addr);
    return fetchstr(addr, buf, max);
}


extern uint64_t sys_getcwd(void);
extern uint64_t sys_pipe(void);
extern uint64_t sys_dup(void);
extern uint64_t sys_dup3(void);
extern uint64_t sys_chdir(void);
extern uint64_t sys_open(void);
extern uint64_t sys_close(void);
extern uint64_t sys_getdents64(void);
extern uint64_t sys_read(void);
extern uint64_t sys_write(void);
// extern uint64_t sys_link(void);
extern uint64_t sys_unlink(void);
extern uint64_t sys_mkdir(void);
extern uint64_t sys_mount(void);
extern uint64_t sys_umount(void);
extern uint64_t sys_fstat(void);


extern uint64_t sys_getpid(void);
extern uint64_t sys_getppid(void);
extern uint64_t sys_wait(void);
extern uint64_t sys_wait4(void);

extern uint64_t sys_exec(void);
extern uint64_t sys_fork(void);
extern uint64_t sys_exit(void);
extern uint64_t sys_yield(void);

extern uint64_t sys_brk(void);
extern uint64_t sys_mmap(void);
extern uint64_t sys_munmap(void);

extern uint64_t sys_uname(void);
extern uint64_t sys_times(void);
extern uint64_t sys_nanosleep(void);
extern uint64_t sys_gettimeofday(void);

extern uint64_t sys_test(void);
extern uint64_t sys_getuid(void);

// 转移表，调用编号为数组下标，对应函数指针
static uint64_t (*syscalls[])(void) = {
    [SYS_getcwd]      sys_getcwd,
    [SYS_pipe2]       sys_pipe,
    [SYS_dup]         sys_dup,
    [SYS_dup3]        sys_dup3,
    [SYS_chdir]       sys_chdir,
    [SYS_openat]      sys_open,
    [SYS_close]       sys_close,
    [SYS_getdents64]  sys_getdents64,
    [SYS_read]        sys_read,
    [SYS_write]       sys_write,
    // [SYS_linkat]      sys_link,
    [SYS_unlinkat]    sys_unlink,
    [SYS_mkdirat]     sys_mkdir,
    [SYS_mount]       sys_mount,
    [SYS_umount2]     sys_umount,
    [SYS_fstat]       sys_fstat,

    [SYS_getpid]      sys_getpid,
    [SYS_getppid]     sys_getppid,
    [SYS_wait]        sys_wait,
    [SYS_wait4]       sys_wait4,
    [SYS_execve]      sys_exec,
    [SYS_exec]        sys_exec,
    [SYS_fork]        sys_fork,
    [SYS_clone]       sys_fork,
    [SYS_exit]        sys_exit,

    [SYS_brk]         sys_brk,
    [SYS_mmap]        sys_mmap,
    [SYS_munmap]      sys_munmap,

    [SYS_uname]       sys_uname,
    [SYS_times]       sys_times,
    [SYS_sched_yield] sys_yield,
    [SYS_gettimeofday] sys_gettimeofday,
    [SYS_nanosleep]   sys_nanosleep,

    [SYS_test]        sys_test,
    [SYS_getuid]      sys_getuid,
    [SYS_geteuid]	  sys_getuid,
	[SYS_getgid]	  sys_getuid,
	[SYS_getegid]	  sys_getuid,
};

uint64_t sys_mount(void)
{
    return 0;
}
uint64_t sys_umount(void)
{
    return 0;
}

uint64_t sys_test(void) 
{
    int n;
    argint(0, &n);
    printk("hello world from proc %d, hart %d, arg %d\n", mytask()->pid, r_tp(), n);
    return 0;
}

uint64_t sys_getuid(void)
{
	return 0;
}


uint64_t sys_exit(void)
{
    int exit_code;
    argint(0, &exit_code);
    //printk("from proc %d, hart %d, exit with %d\n",mytask()->pid, r_tp(), exit_code);
    exit_current_and_run_next(exit_code);
    panic("Unreachable in sys_exit!");
    return 0;
}

uint64_t sys_yield(void)
{
    suspend_current_and_run_next();
    return 0;
}

uint64_t sys_getpid(void)
{
    return mytask()->pid;
}

uint64_t sys_getppid(void)
{
    return mytask()->parent->pid;
}
static void uname(struct utsname *info)
{
    memset(info, 0, sizeof(struct utsname));
    strncpy(info->sysname, "XOS", strlen("XOS")+1);
    strncpy(info->nodename, "stable", strlen("stable")+1);
    strncpy(info->release, "1.0", strlen("1.0")+1);
    strncpy(info->machine,"K210", strlen("K210")+1);
    strncpy(info->domainname, "none", strlen("none")+1);

}

uint64_t sys_uname(void)
{
    uint64_t un;
    if (argaddr(0, &un) < 0) {
        return -1;
    }

    struct utsname u;
    uname(&u);

    // 将　u　拷贝到虚拟地址　un 处
    copyout2(un, (char *)&u, sizeof(u));
    return 0;
}


/*
#define STDIN 0
#define STDOUT 1
#define STDERR 2
*/

uint64_t sys_read(void)
{
    struct file *f;
    int n;
    uint64_t p;

    if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    

    int size = fileread(f, p, n);
    return size;
}

uint64_t sys_write(void)
{
    struct file *f;
    int n;
    uint64_t p;

    if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    //printk("addr :%p\n", p);
    int size = filewrite(f, p, n);
    //printk("w %d\n",size );
    return size;
}

// syscall(SYS_openat, AT_FDCWD, path, flags, O_RDWR);
// #define AT_FDCWD -100
// #define O_RDWR 0x002

//int open(int const char* pathname,int flags, mode_t mode);

//int openat(int dirfd, const char *pathname, int flags, mode_t mode);
//如果pathname指定的是绝对路径名。这种情况下，dirfd参数将被忽略，openat函数等同于open函数。
//如果pathname指定的是相对路径，并且dirfd的值不是AT_FDCWD，pathname则参照dirfd指定的目录下寻找，dirfd可以是当前目录（打开的是当前目录），也可以其他目录。dirfd是通过open函数打开相对路径名所在的目录来获取的。
//如果pathname指定的相对路径名，并且dirfd的值为AT_FDEWD时，相对路径名在当前目录获取（相当于当前目录的相对路径）。

uint64_t sys_open(void)
{
    char path[FAT32_MAX_PATH];
    int flags;
    int fd;
    int mode;


    if(argstr(1, path, FAT32_MAX_PATH) < 0 || argint(0, &fd) < 0)
        return -1;
    if (argint(2, &flags) || argint(3, &mode))
        return -1;



    // printk("path [%s]\n", path);
    // printk("mode :[%p]\n", omode);

    return fileopen(fd, path, flags);
    
}

uint64_t sys_close(void)
{
    int fd;
    struct file *f;

    if(argfd(0, &fd, &f) < 0)
        return -1;


    // char buf[BSIZE];
    // eread(mytask()->ofile[fd]->ep, 0, (uint64_t)buf, 0,sizeof(buf));
    //printk("close %d\n", fd);

    struct TaskControlBlock *p = mytask();
    // acquire(&p->tcb_lock); 
    p->ofile[fd] = NULL; //
    // release(&p->tcb_lock);

    fileclose(f);
    return 0;
}


uint64_t sys_pipe(void)
{
    uint64_t fdarray; // user pointer to array of two integers
    struct file *rf, *wf;
    int fd0, fd1;
    struct TaskControlBlock *p = mytask();

    if(argaddr(0, &fdarray) < 0)
        return -1;
    if(pipealloc(&rf, &wf) < 0)
        return -1;
    fd0 = -1;
    if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
        if(fd0 >= 0)
        p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    // if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
    //    copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    if(copyout2(fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
        copyout2(fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    return 0;
}


// 绝对路径
uint64_t sys_getcwd(void)
{
    uint64_t addr;
    if (argaddr(0, &addr) < 0)
        return -1;

    struct dirent *de = mytask()->cwd;
    char path[FAT32_MAX_PATH];
    char *s;
    int len;

    if (de->parent == NULL) {
        s = "/";
    } else {
        s = path + FAT32_MAX_PATH - 1;
        *s = '\0';
        while (de->parent) {
            len = strlen(de->filename);
            s -= len;
            if (s <= path)          // can't reach root "/"
                return -1;
            strncpy(s, de->filename, len);
            *--s = '/';
            de = de->parent;
        }
    }
    //printk("%s\n", s);

  // if (copyout(myproc()->pagetable, addr, s, strlen(s) + 1) < 0)
    if (copyout2(addr, s, strlen(s) + 1) < 0)
        return 0;
    
    return 1;

}

// 根据已打开的文件描述符产生一个新的文件描述符，新的文件描述符与现有文件描述符执行同一个文件；
// int dup(int oldfd);
uint64_t sys_dup(void)
{
    struct file *f;
    int fd;

    if(argfd(0, 0, &f) < 0)
        return -1;
    if((fd = fdalloc(f)) < 0)
        return -1;
    filedup(f); // 增加原文件描述符的引用计数
    return fd;
}

//int dup2(int oldfd, int newfd);
//将newfd指向oldfd所指的文件，相当于重定向功能。如果newfd已经指向一个已经打开的文件，那么他会首先关闭这个文件，然后在使newfd指向oldfd文件；
//如果newfd和oldfd指向同一个文件，那么不会关闭，直接返回。
uint64_t sys_dup3(void)
{
    struct file *f;
    int oldfd;
    int newfd;

    if(argint(0, &oldfd) || argfd(0, 0, &f) < 0 || argint(1, &newfd))
        return -1;
    
    //printk("old :[%d] -> [%p], new :[%d]\n", oldfd, f, newfd);

    if (oldfd == newfd)
    {
        return newfd; //?
    }
    

    //struct TaskControlBlock *p = mytask();
    // acquire(&p->tcb_lock); 
    //p->ofile[oldfd] = NULL; //
    // //printk("[%d]: [%p]\n", newfd,p->ofile[newfd]);
    // release(&p->tcb_lock);
    //fileclose(f); // 已打开的文件计数不可能为0，标准输入输出计数为0也不会关闭

    newfd = fdalloc2(f, newfd);
    if (newfd != -1)
        filedup(f);
    //printk("%d\n", newfd);
    return newfd;
}

uint64_t sys_fstat(void)
{
    struct file *f;
    struct kstat kst;
    uint64_t st; // user pointer to struct stat

    if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
        return -1;
    memset(&kst, 0, sizeof(struct kstat));

    //strncpy(kst., f->ep->filename, STAT_MAX_NAME);
    kst.st_mode = (f->ep->attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    kst.st_dev = f->ep->dev;
    kst.st_size = f->ep->file_size;

    return copyout2(st, (char *)&kst, sizeof(struct kstat));

    //return filestat(f, st);
}


uint64_t sys_unlink(void)
{
    int fd;
    char filepath[FAT32_MAX_FILENAME];
    int flags;
    if (argint(0, &fd) < 0 || argstr(1, filepath, FAT32_MAX_FILENAME) < 0 || argint(2, &flags) < 0) {
        return -1; 
    }

    struct dirent *ep = ename(filepath);
    if (--ep->ref == 0)
        eremove(ep);


    return 0;


}


uint64_t sys_getdents64(void)
{
    struct file *f;
    uint64_t p;
    int size;

    if(argfd(0, 0, &f) < 0 || argaddr(1, &p) < 0 || argint(2, &size) < 0)
        return -1;
    
    // printk("filename :[%s]\n", f->ep->filename);
    // printk("off in parent dir entry: [%d]\n", f->ep->off);
    // printk("type 0x10 : [%d]\n",f->ep->attribute);
    
    int n = ls(f, p, size);

    return n;


}


uint64_t sys_gettimeofday() 
{
    uint64_t ts;
    int tz;
    if (argaddr(0, &ts) < 0 || argint(1, &tz)) {
        return -1;
    }
    TimeVal tm = tm2tamp(sys_time_get(), tz);
 
    //printk("[%d]\n", (tm.sec & 0xffff) * 1000 + tm.usec / 1000);
    // 将tm 拷贝到　虚拟地址ts所在地方

    copyout2(ts, (char *)&tm, sizeof(tm));

    return 0;
}

uint64_t sys_nanosleep()
{
    uint64_t tv;
    if (argaddr(0, &tv) < 0) {
        return -1;
    }
    // 这里参数１和参数２一样，原本１是所需睡眠时间，２是返回剩余时间（可能被唤醒？）


    TimeVal tm;
    // 将 虚拟地址tv的内容　拷贝到tm
    copyin2((char *)&tm, tv, sizeof(tm));

    if (tm.sec >= 1000000000L || tm.sec < 0)
		return -1;

    uint32_t t = ksleep(tm.sec);
    tm.sec = t;

    copyout2(tv, (char *)&tm, sizeof(tm));

    return t;
}



uint64_t sys_brk(void) 
{
    uint64_t addr;
    if (argaddr(0, &addr) < 0) {
        return -1;
    }
    printk("brk %p\n", addr);
    struct TaskControlBlock *task = mytask();
    if (addr == 0)
        return task->base_size;
    brk(addr - task->base_size);
    return task->base_size;
}


uint64_t sys_times(void) 
{
    struct tms tm;
    uint64_t va;
    int start = TICKS;
    if (argaddr(0, &va) < 0) {
        return -1;
    }
    getTimes(&tm);


    copyout2(va, (char *)&tm, sizeof(tm));

    return TICKS - start;
}

uint64_t sys_mkdir(void) 
{
    int fd;
    int mode;
    struct dirent *ep;
    char path[FAT32_MAX_FILENAME];
    if (argint(0, &fd) < 0 || argstr(1, path, FAT32_MAX_FILENAME) < 0 || argint(2, &mode) || (ep = create(path, T_DIR, 0)) == 0) {
        return -1;
    }
    //mkdir(fd, path);
    eunlock(ep);
    eput(ep);
    return 0;
}

uint64_t sys_chdir(void) 
{
    char path[FAT32_MAX_PATH];
    struct dirent *ep;
    struct TaskControlBlock *p = mytask();
    
    if(argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = ename(path)) == NULL){
        return -1;
    }
    elock(ep);
    if(!(ep->attribute & ATTR_DIRECTORY)){
        eunlock(ep);
        eput(ep);
        return -1;
    }
    eunlock(ep);
    eput(p->cwd);
    p->cwd = ep;
    return 0;
}



uint64_t sys_fork(void)
{
    printk("fork\n");
    // int flags;
    // uint64_t stack;
    // if (argint(0, &flags) < 0 || argaddr(1, &stack) < 0) {
    //     return -1;
    // }


    struct TaskControlBlock *p = mytask();

    struct TaskControlBlock *newT = fork(p);


    // 调用之前就将 trap->pec　+ 4　，无需改变
    // 父子进程回到用户态的瞬间都处于刚刚从一次系统调用返回的状态，但二者的返回值不同。
    //将子进程的 Trap 上下文用来存放系统调用返回值的 a0 寄存器修改为 0 ，而父进程系统调用的返回值会在 trap_handler 中 syscall 返回之后再设置为 sys_fork 的返回值，这里我们返回子进程的 PID 。
    // 这就做到了父进程 fork 的返回值为子进程的 PID ，而子进程的返回值则为 0 。通过返回值是否为 0 可以区分父子进程。

    struct context *trap_cx = (void *)ppn2pa(newT->trap_cx_ppn);
    trap_cx->gr.a0 = 0;

    // if (stack != 0)
    //     trap_cx->gr.sp = stack;

    task_add(newT);


    return newT->pid;
}


/*
int execve(const char *name, char *const argv[], char *const argp[])
{
    return syscall(SYS_execve, name, argv, argp);
}
    char *newargv[] = {"test_echo", NULL};
    char *newenviron[] = {NULL};
    execve("test_echo", newargv, newenviron);
*/
uint64_t sys_exec(void)
{

    char path[FAT32_MAX_PATH];
    //int i;
    uint64_t argv, envp;

    if(argstr(0, path, FAT32_MAX_PATH) < 0 || argaddr(1, &argv) < 0 || argaddr(2, &envp)){
        return -1;
    }
    //memset(argv, 0, sizeof(argv));

    printk("path %s\n", path);
    uint64_t argc, argp;
    //int arglen;
    char **utable = (char **)argv;
    for (argc = 0; utable; argc++) 
    {
        if (copyin2((char *)&argp, (uint64_t)(utable + argc), sizeof(&argp)) < 0)
            break;
        if (argp == 0)
            break;
        
        char *buf = buddy_alloc(HEAP_ALLOCATOR, 4096);
        if (copyinstr2(buf, argp, 4096) == -1)
        {
            buddy_free(HEAP_ALLOCATOR,buf);

            break;
        }
        printk("argc :%s\n", buf);
        buddy_free(HEAP_ALLOCATOR,buf);
    }


    execve(path, (char **)argv, (char **)envp);

    panic("stop !\n");

    return 0;
}


uint64_t sys_wait4(void)
{
    int pid;
    uint64_t uaddr;
    int opt;
    if (argint(0, &pid) < 0 || argaddr(1, &uaddr) < 0 || argint(2, &opt))
        return -1;

    //printk("wait pid [%d], opt :[%d]\n",pid, opt);

    uint64_t p =  waitpid(pid, uaddr, opt);
    //printk("%d\n", p);
    return p;

}
uint64_t sys_wait(void)
{

    uint64_t uaddr;

    if (argaddr(0, &uaddr) < 0)
        return -1;

    printk("wait pid [%d]\n",-1);

    uint64_t p =  waitpid(-1, uaddr, 0);
    //printk("%d\n", p);
    return p;

}

//syscall(SYS_mmap, start, len, prot, flags, fd, off);


//void *mmap(void *start, size_t length, int prot, int flags,int fd, off_t offset);
//start：映射区的开始地址。
//length：映射区的长度。

//prot：期望的内存保护标志，不能与文件的打开模式冲突
//PROT_EXEC //页内容可以被执行
//√　PROT_READ  //页内容可以被读取
//√　PROT_WRITE //页可以被写入
//PROT_NONE  //页不可访问
//PORT_GROWSDOWN //用于堆栈，映射区可以向下扩展。
//PORT_GROWSUP  // 向上扩展

//flag
//MAP_FIXED //使用指定的映射起始地址，如果由start和len参数指定的内存区重叠于现存的映射空间，重叠部分将会被丢弃。如果指定的起始地址不可用，操作将会失败。并且起始地址必须落在页的边界上。
//√　MAP_SHARED //与其它所有映射这个对象的进程共享映射空间。对共享区的写入，相当于输出到文件。直到msync()或者munmap()被调用，文件实际上不会被更新。
//MAP_PRIVATE //建立一个写入时拷贝的私有映射。内存区域的写入不会影响到原文件。这个标志和以上标志是互斥的，
//MAP_ANONYMOUS //匿名映射，映射区不与任何文件关联。
//√　MAP_FILE //兼容标志

//fd：有效的文件描述（已经打开）。如果MAP_ANONYMOUS被设定，为了兼容问题，其值应为-1。
//offset：被映射对象内容的起点。
//成功执行时，mmap()返回被映射区的指针,失败时返回MAP_FAILED[其值为(void *)-1]
uint64_t sys_mmap(void)
{
    uint64_t addr;
    
    int prot, flags, fd;
    uint64_t offset, length;
    if(argaddr(0, &addr) < 0 || argaddr(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argaddr(5, &offset) < 0){
        return -1;
    }

    // if(addr != 0) //start = NULL表示地址由内核指定
    //     panic("mmap: addr not 0");
    // if(offset != 0)
    //     panic("mmap: offset not 0　[%d]\n", offset);

    struct TaskControlBlock *p = mytask();
    struct file* f = p->ofile[fd];
    struct MapArea *ma = NULL;


    MapPermission perm = VM_U;
    if (prot & PROT_WRITE) {
        if(!f->writable && !(flags & MAP_PRIVATE)) return -1; // map to a unwritable file with PROT_WRITE
        perm |= VM_W;
    }
    if (prot & PROT_READ) {
        if(!f->readable) return -1; // map to a unreadable file with PROT_READ
        perm |= VM_R;
    }



    //f->ep->file_size; -- length
    

    if (list_empty(&p->memory_set->vma))
    {
        ma = ma_new((VirtAddr)VMA_START, (VirtAddr)(VMA_START + length), Framed, perm);
        list_append(&p->memory_set->vma, &ma->tag);
    }
    else
    {
        struct MapArea *t = elem2entry(struct MapArea, tag, list_tail(&p->memory_set->vma));
        ma = ma_new(vpn2va(t->vm_end), (VirtAddr)(vpn2va(t->vm_end) + length), Framed, perm);
        list_append(&p->memory_set->vma, &ma->tag);
    }
    ma->length = length;
    f->off = 0;
    ma->off = offset;
    ma->f = f;
    ma->flags = flags;
    filedup(f);

    // 已经实现缺页处理
    // printk("vpn s :[%p]\n", ma->vm_start);
    // printk("return %p\n", vpn2va(ma->vm_start));

    return vpn2va(ma->vm_start);

}
//int munmap(void *addr, size_t length);
//syscall(SYS_munmap, start, len);
// 成功返回0 ,失败返回-1
// errno:
//EACCES：访问出错
//EAGAIN：文件已被锁定，或者太多的内存已被锁定
//EBADF：fd不是有效的文件描述词
//EINVAL：一个或者多个参数无效
//ENFILE：已达到系统对打开文件的限制
//ENODEV：指定文件所在的文件系统不支持内存映射
//ENOMEM：内存不足，或者进程已超出最大内存映射数量
//EPERM：权能不足，操作不允许
//ETXTBSY：已写的方式打开文件，同时指定MAP_DENYWRITE标志
//SIGSEGV：试着向只读区写入
//SIGBUS：试着访问不属于进程的内存区

uint64_t sys_munmap(void)
{
    uint64_t addr;
    int length;
    if(argaddr(0, &addr) < 0 || argint(1, &length) < 0){
        return -1;
    }
    //　ｕｎｃｏｍｐｌｅｔｅ

    // 判断是否mmap
    // 在vma链表找到对应　结点

    // 写回
    // unmap
    // 关文件

    return 0;
}


void syscall_handler(struct context *f)
{
    printk("ecall from user!  a7 [%d]\n", f->gr.a7);

    struct context *ff = mytask_trap_cx(); 
    uint64_t result = 0;
    ff->epc += 4;
    if (ff->gr.a7 > 0 && syscalls[ff->gr.a7])
    {
        result = syscalls[ff->gr.a7]();
    }
    else
    {
        printk("unknow syscall [%d]", ff->gr.a7);
    }

    ff = mytask_trap_cx();
    ff->gr.a0 = result;
        

}




