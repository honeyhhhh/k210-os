## XOS
XOS 是一个RISC-V 64位操作系统       
它主要使用C语言以及汇编语言编写         
实现主要参考于 [rCore](https://rcore-os.github.io/rCore-Tutorial-Book-v3/) 和　[xv6-k210](https://github.com/HUST-OS/xv6-k210)     


## ps
文档没来得及整理...

## 项目结构
.                   
├── bootloader              (SBI文件)               
│   ├── fw_jump.bin                 
│   ├── fw_jump.elf                 
│   ├── opensbi_asm                     
│   ├── rustsbi-k210        (Platform: K210 (Version 0.2.0 -alpha.2))               
│   ├── rustsbi-k210.bin                    
│   ├── rustsbi-qemu                
│   └── rustsbi-qemu.bin                
├── doc                     (文档 -- 未完成)                
│   ├── init.txt                
│   ├── rustsbi.md          
│   └── 内存管理.md                     
├── Makefile                                
├── README.md                        
├── src                 
│   ├── k210.ld         (K210　链接脚本　指定内核内存布局以及一些标号的定义)                            
│   ├── linker.ld       (QEMU 链接脚本)                             
│   ├── sbi.c           (sbi - ecall)                               
│   ├── entry_k210.S    (K210 定义入口函数，　栈初始化以及定义一些全局标号作为C语言全局指针变量使用)                    
│   ├── entry.S         (QEMU 定义入口函数)                         
│   ├── main.c          (内核　主函数，　做一些初始化工作)                      

│   ├── list.c          (数据结构　——　双向链表)                        
│   ├── max_heap.c      (数据结构　——　简单最大堆)                      
│   ├── bitmap.c        (数据结构　-- 位图)                         
│   ├── bitmap_buddy.c  (数据结构　-- 伙伴分配器)                       

│   ├── stdio.c         (标准输入输出 格式化输出printk)                         
│   ├── string.c        (内存拷贝函数，字符串函数)                  
│   ├── panic.c         (panic, warn)                                   
│   ├── console.c       (控制台　输入输出)                              
│   ├── timer.c         (时钟中断)                              
│   ├── sleep.c         (休眠函数)                                          

│   ├── mm_frame_allocator.c    (内存管理　--　页分配器(管理范围ekernel ~ MEMORYEND0x80600000))                     
│   ├── mm_heap_allocator.c     (内存管理　--  动态内存分配(最小分配8字节, 管理bss内核临时堆512KB))                     
│   ├── mm_memoryset.c          (内存管理　--  地址空间(一系列逻辑段), 进程地址空间初始化)                              
│   ├── mm_page_table.c         (内存管理  --  页表)                                



│   ├── sync_sem.c      (同步，　信号量　取代xv6的sleeplock)                                        
│   ├── sync_spinlock.c (同步，　自旋锁)                                

│   ├── syscall.c       (系统调用)                          

│   ├── tk_manage.c     (进程管理　-- 任务管理器(就绪队列，休眠队列(用于wait/waitpid)，所有进程的队列))             
│   ├── tk_pid.c        (进程管理 -- 进程标识符和内核栈　及其分配)                          
│   ├── tk_proc.c       (进程管理 -- 处理器管理、init进程初始化、进程调度)                          
│   ├── tk_switch.S     (进程管理　-- 任务切换　栈)                         
│   └── tk_task.c       (进程管理 -- 任务控制块, 进程生成与回收)                                        

│   ├── exception.c     (中断分发，处理)                    
│   ├── exception_entry.S　(中断保存上下文、切换地址空间)                   

│   ├── d_clint.c          (驱动)                       
│   ├── d_dmac.c            
│   ├── d_fpioa.c           
│   ├── d_gpiohs.c          
│   ├── d_rtc.c             
│   ├── d_sdcard.c          
│   ├── d_spi.c             
│   ├── d_sysctl.c              
│   ├── d_utils.c                   

│   ├── fs_buf.c        (文件系统　-- 缓冲层)           
│   ├── fs_disk.c       (文件系统 -- )              
│   ├── fs_fat32.c      (文件系统 -- FAT32文件系统)             
│   ├── fs_file.c               
│   ├── fs_pipe.c               

│   ├── include         (头文件)                    
│   │   ├── assert.h            
│   │   ├── bitmap_buddy.h          
│   │   ├── bitmap.h            
│   │   ├── buf.h           
│   │   ├── clint.h             
│   │   ├── console.h               
│   │   ├── disk.h              
│   │   ├── dmac.h              
│   │   ├── elf.h               
│   │   ├── exception.h         
│   │   ├── file.h              
│   │   ├── fpioa.h             
│   │   ├── fs.h            
│   │   ├── gpiohs.h            
│   │   ├── list.h          
│   │   ├── max_heap.h          
│   │   ├── mm.h                
│   │   ├── pipe.h          
│   │   ├── proc.h          
│   │   ├── riscv_asm.h             
│   │   ├── rtc.h               
│   │   ├── sbi.h               
│   │   ├── sdcard.h                
│   │   ├── sem.h               
│   │   ├── sleep.h             
│   │   ├── spi.h               
│   │   ├── spinlock.h                  
│   │   ├── stddef.h            
│   │   ├── stdio.h             
│   │   ├── string.h            
│   │   ├── syscall.h                   
│   │   ├── sysctl.h            
│   │   ├── sysinfo.h           
│   │   ├── timer.h         
│   │   └── utils.h             

├── target                  
└── tools               (K210 烧录工具)                            
    ├── flash-list.json             
    └── kflash.py               










