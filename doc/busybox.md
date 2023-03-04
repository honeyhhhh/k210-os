## 阶段总结与展望
从开始的啥也不会，到现在能看得懂一点，从无到有，收获很大!       

由于期末考和考研的压力，不能再投入大量精力到这个比赛的决赛阶段，但还是会抽出时间来尝试！        


## busybox      
busybox是一个集成了一百多个最常用linux命令和工具的软件，只有１M左右大小，相当于一个精简的shell      

busybox初始化:      

main函数执行流程：          
busybox 1.22.1      
/libb/appletlib.c
```c
int main(int argc UNUSED, char **argv)
{
    ...
	if (argv[1] && strncmp(argv[0], "busybox", 7) == 0) {
		/* "busybox <applet> <params>" should still work as expected */
		argv++;
	}
    ...
	applet_name = argv[0];
    ...
	applet_name = bb_basename(applet_name);
    run_applet_and_exit(applet_name, argv);
}
```

```c
void FAST_FUNC run_applet_and_exit(const char *name, char **argv)
{
	int applet = find_applet_by_name(name);   // 获得调用号
	if (applet >= 0)
		run_applet_no_and_exit(applet, argv);
	if (strncmp(name, "busybox", 7) == 0)
		exit(busybox_main(argv)); //busybox_main 判断了无参调用、--help、--list等参数 , 最终也是调用run_applet_no_and_exit
}
```

```c
void FAST_FUNC run_applet_no_and_exit(int applet_no, char **argv)
{
	int argc = 1;

	while (argv[argc])
		argc++;         //计算参数个数

    ...
	exit(applet_main[applet_no](argc, argv));
}
```

其中applet_main在busybox.h有如下：              
```c
/* Defined in appletlib.c (by including generated applet_tables.h) */
/* Keep in sync with applets/applet_tables.c! */
extern const char applet_names[] ALIGN1;
extern int (*const applet_main[])(int argc, char **argv);
```
其中applet_names和applet_main 是在include/applet_tables.h中定义的，该文件是根据配置结果动态生成的               
两者一一对应，根据命令名称计算所以呢，然后根据索引找到对应命令的入口函数


busybox与shell：                
shell　命令解释器，本质是一个可执行文件。命令可以分为内置命令(由 Bash 自身提供的命令，而不是文件系统中的某个可执行文件, 相当于调用一个函数, 如cd、echo、pwd、test)　和　外置命令(是在/usr/bin、/bin下的一个个可执行文件， 会触发磁盘IO，需要单独fork出一个进程，然后exec, 如ls、vim、cat)       

外置命令虽然功能丰富强大，但是体积也大，在存储容量有限的系统中，可以使用busybox来代替shell和常用的外置命令          

busybox中也包含了shell, 常用的有ash_main        


## 关于init进程
这里借鉴了上海海洋大学　许春林同学的代码            
```c
#include "unistd.h"

char *argvs1[] = {"./busybox", "sh", "./lua_testcode.sh", NULL};
char *argvs2[] = {"./busybox", "sh", "./busybox_testcode.sh", NULL};
char *envps[] = {"SHELL=shell", "PWD=/root", "HOME=/root", "USER=root", "SHLVL=1", "PATH=/root", "OLDPWD=/root", "_=busybox", NULL};

int main(int argc, char *argv)
{
    int pid = fork();
    if(pid)
    {
        waitpid(pid, 0, 0);
        execve("./busybox", argvs2, envps);
    } else {
        execve("./busybox", argvs1, envps);
    }
    return 0;
}
```

将elf可执行文件生成二进制机器码的方法:
1、使用objdump转化为二进制文件　 $(OBJCOPY) $U/app --strip-all -O binary $U/app.bin
2、使用xxd命令生成c语言风格的数组　xxd -g 1 -i $U/app.bin > $U/app_hex_c.c


## exec系列系统调用
要主动运行busybox，首先需要载入busybox的ELF可执行文件, 所以完善exec系统调用是本阶段的一个重点           

ELF 加载器解析 ELF 文件，映射内存中的各种程序段，设置入口点并初始化进程堆栈。它将 ELF 辅助向量与其他信息（如 argc、argv、envp）一起放在进程堆栈上。初始化后，进程的堆栈如下所示：
// STACK TOP
//      argc　= number of args    8 bytes
//      *argv [] (with NULL as the end) 8 bytes each
//      *envp [] (with NULL as the end) 8 bytes each
//      auxv[] (with NULL as the end) 16 bytes each: now has PAGESZ(6)
//      padding (16 bytes-align)
//      rand bytes: Now set 0x00 ~ 0x0f (not support random) 16bytes
//      String: platform "RISC-V64"
//      [padding for align]   (sp - (get_random_int() % 8192)) & (~0xf)
//      Argument string(argv[])
//      Environment string (envp[]):
// STACK BOTTOM


ELF 辅助向量Auxiliary Vectors是一种将某些内核级信息传输到用户进程的机制。   

3 AT_PHDR :load_addr + elf.phoff (load_addr指向可执行程序加载的起始位置, phoff 为程序头部在文件中的偏移)
6 AT_PAGESZ : mmap 

## 写时复制COW          
即使busybox的大小只有1M+，但是由于本系统的内存分配算法为伙伴系统，分配的内存大小需要为2^n，并且k210的可用内存只有(6+2)M，所以当busybox调用fork系统调用时是无法满足的。而且拷贝整个地址空间是十分耗时的，并且在很多情况下，程序立即调用exec函数来替换掉地址空间，导致fork做了很多无用功。即使不调用exec函数，父进程和子进程的代码段等只读段也是可以共享的，从而达到节省内存空间的目的。          
所以使用Copy-On-Write可以避免不必要的拷贝来解决内存空间不够的问题，也可以将地址空间拷贝的耗时进行延迟分散，提高操作系统的效率。         

对fork函数进行修改，使其不对地址空间进行拷贝            
将父子进程页面的页表项都设置为不可写，并设置COW标志位（在页表项中保留了2位给操作系统，这里用的是第8位）　#define PTE_COW (1L << 8)          
设置一个数组用于保存内存页面的引用计数，使其只有在引用计数为1的时候释放页面，其他时候就只减少引用计数           

在cowfork函数中先判断COW标志位，当该页面是COW页面时，就可以根据引用计数来进行处理。如果计数大于1，那么就需要通过kalloc申请一个新页面，然后拷贝内容，之后对该页面进行映射，映射的时候清除COW标志位，设置PTE_W标志位；而如果引用计数等于1，那么就不需要申请新页面，只需要对这个页面的标志位进行修改就可以了


## 懒分配       
懒分配（lazy allocation）。         
用户态程序通过sbrk系统调用来在堆上分配内存，而sbrk则会通过kalloc函数来申请内存页面，之后将页面映射到页表当中。          
当申请小的空间时，上述过程是没有问题的。但是如果当进程一次申请很大的空间，如数GB的空间，再使用上述策略来一页页地申请映射的话就会非常的慢，这时候就引入了lazy allocation技术，当调用sbrk时不进行页面的申请映射，而是仅仅增大堆的大小，当实际访问页面时，就会触发缺页异常，此时再申请一个页面并映射到页表中，这是再次执行触发缺页异常的代码就可以正常读写内存了。         
通过lazy allocation技术，就可以将申请页面的开销平摊到读写内存当中去，在sbrk中进行大量内存页面申请的开销是不可以接受的，但是将代价平摊到读写操作当中去就可以接受了。         

## 缺页异常
缺页指的是进程访问页面时页面不在页表中或在页表中无效的现象，此时 



可以利用缺页来实现　Lazy 策略   