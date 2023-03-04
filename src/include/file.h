#ifndef __FILE_H
#define __FILE_H

#include "stddef.h"
#include "spinlock.h"

#define NOFILE      101
// open files per process
#define NFILE       128  
// open files per system
#define NINODE       50  
// maximum number of active i-nodes
#define NDEV         10  
// maximum major device number

#define AT_FDCWD -100


#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define PROT_GROWSDOWN 0X01000000
#define PROT_GROWSUP 0X02000000

#define MAP_FILE 0
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0X02
#define MAP_FAILED ((void *) -1)

struct file {
    enum { FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE } type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct dirent *ep;
    uint off;          // FD_ENTRY
    short major;       // FD_DEVICE
};

// #define major(dev)  ((dev) >> 16 & 0xFFFF)
// #define minor(dev)  ((dev) & 0xFFFF)
// #define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64_t, int);
  int (*write)(int, uint64_t, int);
};
struct ftable
{
    struct spinlock lock;
    struct file file[NFILE];
};

extern struct devsw devsw[];
extern struct ftable ftable;

#define CONSOLE 1

struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64_t, int n);
int             filestat(struct file*, uint64_t addr);
int             filewrite(struct file*, uint64_t, int n);
int             dirnext(struct file *f, uint64_t addr);


int fileopen(int dfd, char *path, int omode);
int fdalloc(struct file *f);
int fdalloc2(struct file *f, int newfd);
struct dirent* create(char *path, short type, int mode);
/*
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;
这个结构里面保存所有已经打开的文件。以下函数对其操作:
struct file* filealloc(void);
struct file* filedup(struct file *f);
void fileclose(struct file *f);
int filestat(struct file *f, struct stat *st);
int fileread(struct file *f, char *addr, int n);
int filewrite(struct file *f, char *addr, int n);

fileread() 先判断文件对应的是 pipe 还是 inode，分别做处理。 如果是 inode，则使用 readi() 来读取数据，并修改 file 读取后的文件偏移量:

除了全局的 ftable，每个进程中也保存了自己使用的文件信息:
struct proc {
  ...
  struct file *ofile[NOFILE];
  ...
};
proc.ofile[i] 的 index i 就是返回给用户的 File Descriptor，里面的指针指向 ftable 里的文件。

对于sys_open
如果是一个新文件，create() 会在磁盘上标记一个 dinode 为使用，并返回其指针。然后使用 filealloc() 在 ftable 中分配一个 file， 最后使用 fdalloc() 将 filealloc() 返回的 file 指针设置给进程的 proc.ofile[i]，最后将 i 作为 fd 返回。
*/

#endif