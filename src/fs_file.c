
#include "include/stddef.h"
#include "include/fs.h"
#include "include/file.h"
#include "include/spinlock.h"
#include "include/sem.h"
#include "include/string.h"
#include "include/stdio.h"
#include "include/assert.h"
#include "include/proc.h"
#include "include/pipe.h"



struct devsw devsw[NDEV];
struct ftable ftable;

void
fileinit(void)
{
    initlock(&ftable.lock, "ftable");
    struct file *f;
    for(f = ftable.file; f < ftable.file + NFILE; f++){
        memset(f, 0, sizeof(struct file));
    }
    for (int i = 0; i < 3; i++)
    {
        ftable.file[i].major = 1;
        ftable.file[i].ref = 1;
        ftable.file[i].type = FD_DEVICE;
        ftable.file[i].writable = 1;
        ftable.file[i].readable = 1;
    }


    //printk("fileinit\n");

}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return NULL;
}


// 只是增加引用计数s
struct file*
filedup(struct file *f)
{
    acquire(&ftable.lock);
    if(f->ref < 1)
        panic("filedup");
    f->ref++;
    release(&ftable.lock);
    return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
    struct file ff;

    acquire(&ftable.lock);
    if(f->ref < 1)
        panic("fileclose");
    if(--f->ref > 0){
        release(&ftable.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if(ff.type == FD_PIPE){
        pipeclose(ff.pipe, ff.writable);
    } else if(ff.type == FD_ENTRY){
        eput(ff.ep);
    } else if (ff.type == FD_DEVICE) {

    }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64_t addr)
{
    // struct proc *p = myproc();
    struct stat st;
   //struct kstat st;
    
    if(f->type == FD_ENTRY){
        elock(f->ep);
        estat(f->ep, &st);
        eunlock(f->ep);
        // if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
        if(copyout2(addr, (char *)&st, sizeof(st)) < 0)
        return -1;
        return 0;
    }
    return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64_t addr, int n)
{
    int r = 0;

    if(f->readable == 0)
        return -1;
    //printk("type :%d\n", f->type);
    switch (f->type) {
        case FD_PIPE:
            r = piperead(f->pipe, addr, n);
            break;
        case FD_DEVICE:
            if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
                return -1;
            r = devsw[f->major].read(1, addr, n);
            break;
        case FD_ENTRY:
            elock(f->ep);
            //printk("%s\n", f->ep->filename);
            if((r = eread(f->ep, 1, addr, f->off, n)) > 0)
            {
                f->off += r;
            }
            eunlock(f->ep);
            break;
        default:
            panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64_t addr, int n)
{
    //printk("%p\n%p\n%p\n", f->major, devsw[f->major].write, f->type);
    int ret = 0;

    if(f->writable == 0)
        return -1;

    if(f->type == FD_PIPE){
        ret = pipewrite(f->pipe, addr, n);
    } else if(f->type == FD_DEVICE){
        if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
        {
            return -1;
        }
        ret = devsw[f->major].write(1, addr, n);

    } else if(f->type == FD_ENTRY){
        elock(f->ep);
        if (ewrite(f->ep, 1, addr, f->off, n) == n) {
            ret = n;
            f->off += n;
        } else {
            ret = -1;
        }
        eunlock(f->ep);
    } else {
        panic("filewrite");
    }

    return ret;
}

// Read from dir f.
// addr is a user virtual address.
int
dirnext(struct file *f, uint64_t addr)
{
  // struct proc *p = myproc();

    if(f->readable == 0 || !(f->ep->attribute & ATTR_DIRECTORY))
        return -1;

    struct dirent de;
    struct stat st;
    int count = 0;
    int ret;
    elock(f->ep);
    while ((ret = enext(f->ep, &de, f->off, &count)) == 0) {  // skip empty entry
        f->off += count * 32;
    }
    eunlock(f->ep);
    if (ret == -1)
        return 0;

    f->off += count * 32;
    estat(&de, &st);
    // if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
    if(copyout2(addr, (char *)&st, sizeof(st)) < 0)
        return -1;

    return 1;
}


struct dirent*
create(char *path, short type, int mode)
{
    struct dirent *ep, *dp;
    char name[FAT32_MAX_FILENAME + 1];

    if((dp = enameparent(path, name)) == NULL)
        return NULL;

    
    

    if (type == T_DIR) {
        mode = ATTR_DIRECTORY;
    } else if (mode & O_RDONLY) {
        mode = ATTR_READ_ONLY;
    } else {
        mode = 0;  
    }

    elock(dp);
    if ((ep = ealloc(dp, name, mode)) == NULL) 
    {
        eunlock(dp);
        eput(dp);
        return NULL;
    }
  
    if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == T_FILE && (ep->attribute & ATTR_DIRECTORY))) 
    {
        eunlock(dp);
        eput(ep);
        eput(dp);
        return NULL;
    }

    eunlock(dp);
    eput(dp);

    elock(ep);
    return ep;
}

int
fdalloc(struct file *f)
{
    int fd;
    struct TaskControlBlock *p = mytask();

    for(fd = 3; fd < NOFILE; fd++)
    {
        if(p->ofile[fd] == NULL)
        {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

int fdalloc2(struct file *f, int newfd)
{
    struct TaskControlBlock *p = mytask();

    //printk("newfd :[%d] p->ofile[newfd]:[%p]\n",newfd, p->ofile[newfd]);
    if(p->ofile[newfd] == NULL)
    {
        p->ofile[newfd] = f;
        return newfd;
    }

    return -1;    
}



int fileopen(int dfd, char *path, int omode)
{
    int fd;
    struct file *f;
    struct dirent *ep = NULL;

    if (dfd == AT_FDCWD)
    {
        //printk("open !\n");
        if(omode & O_CREATE)
        {
            ep = create(path, T_FILE, omode);
            if(ep == NULL)
                return -1;
        } 
        else 
        {
            if((ep = ename(path)) == NULL)
                return -1;
            elock(ep);
            // if ( ((ep->attribute & ATTR_DIRECTORY)) &&           omode != O_RDONLY || omode != O_DIRECTORY) )
            // {
            //     eunlock(ep);
            //     eput(ep);
            //     return -1;
            // }
        }
        //printk("%p\n", ep);
        if((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
        {
            if (f) {
                fileclose(f);
            }
            eunlock(ep);
            eput(ep);
            return -1;
        }

        if(!(ep->attribute & ATTR_DIRECTORY) && (omode & O_TRUNC))
        {
            etrunc(ep);
        }
        //printk("alloc fd[%d]\n", fd);

        mytask()->ofile[fd] = f; //?

        f->type = FD_ENTRY;
        f->off = (omode & O_APPEND) ? ep->file_size : 0;
        f->ep = ep;
        f->readable = !(omode & O_WRONLY);
        f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

        eunlock(ep);
    }
    else
    {
        //printk("openat !\n");
        f = mytask()->ofile[dfd];
        //printk("%p\n", f->ep);
        // if ((ep = dirlookup(f->ep, path, 0)) == 0) { //这里的ｐａｔｈ需要处理skip
        //   printk("no found [%s] in dir [%s]\n", path, f->ep->filename);
        //     eunlock(f->ep);
        //     eput(f->ep);
        //     return -1;
        // }
        // printk("found [%s] in dir [%s]\n", path, f->ep->filename);
        if(omode & O_CREATE)
        {
            //ep = create(path, T_FILE, omode);
            int type = T_FILE;
            int mode;
            if (type == T_DIR) {
                mode = ATTR_DIRECTORY;
            } else if (mode & O_RDONLY) {
                mode = ATTR_READ_ONLY;
            } else {
                mode = 0;  
            }

            elock(f->ep);
            if ((ep = ealloc(f->ep, path, mode)) == NULL) 
            {
                eunlock(f->ep);
                eput(f->ep);
                return -1;
            }
  
            if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
                (type == T_FILE && (ep->attribute & ATTR_DIRECTORY))) 
            {
                eunlock(f->ep);
                eput(ep);
                eput(f->ep);
                return -1;
            }

            eunlock(f->ep);
            // eput(dp);

            // elock(ep);
            if(ep == NULL)
                return -1;
        } 



        elock(ep);

        if((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
        {
            if (f) {
                fileclose(f);
            }
            eunlock(ep);
            eput(ep);
            return -1;
        }

        if(!(ep->attribute & ATTR_DIRECTORY) && (omode & O_TRUNC))
        {
            etrunc(ep);
        }
        //printk("alloc fd[%d]\n", fd);

        mytask()->ofile[fd] = f; //?

        f->type = FD_ENTRY;
        f->off = (omode & O_APPEND) ? ep->file_size : 0;
        f->ep = ep;
        f->readable = !(omode & O_WRONLY);
        f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

        eunlock(ep);


    }


    return fd;

}