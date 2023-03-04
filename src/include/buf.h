#ifndef __BUF_H
#define __BUF_H

#include "sem.h"
#include "fs.h"

struct buf 
{
    int valid;      // has data been read from disk?
    int disk;		// does disk "own" buf? 
    uint32_t dev;
    uint32_t sectorno;	// sector number 

    struct semaphore lock;

    uint32_t refcnt;    // reference count
    struct buf *prev;
    struct buf *next;
    uint8_t data[BSIZE];

    //uint32_t timestamp; //LRU
};

void            binit(void);
struct buf*     bread(uint32_t, uint32_t);
void            brelse(struct buf *r);
void            bwrite(struct buf *w);
void            buf_test();

/*
块缓冲有两个任务：
（1）同步对磁盘的访问，使得对于每一个块，同一时间只有一份拷贝放在内存中并且只有一个内核线程使用这份拷贝；
（2）缓存常用的块以提升性能。

bread 从磁盘中取出一块放入缓冲区,调用 bget 获得指定扇区的缓冲区
bwrite 把缓冲区中的一块写到磁盘上正确的地方。
当内核处理完一个缓冲块之后，需要调用 brelse 释放它。

块缓冲仅允许最多一个内核线程引用它，以此来同步对磁盘的访问，如果一个内核线程引用了一个缓冲块，但还没有释放它，那么其他调用 bread 的进程就会阻塞。文件系统的更高几层正是依赖块缓冲层的同步机制来保证其正确性。
块缓冲有固定数量的缓冲区，这意味着如果文件系统请求一个不在缓冲中的块，必须换出一个已经使用的缓冲区。这里的置换策略是 LRU，因为我们假设假设最近未使用的块近期内最不可能再被使用。
块缓冲是缓冲区的双向链表。

binit 会从一个静态数组 buf 中构建出一个有 NBUF 个元素的双向链表。所有对块缓冲的访问都通过链表而非静态数组。

一个缓冲区有三种状态：
B_VALID 意味着这个缓冲区拥有磁盘块的有效内容。
B_DIRTY 意味着缓冲区的内容已经被改变并且需要写回磁盘。
B_BUSY 意味着有某个内核线程持有这个缓冲区且尚未释放。

bget 扫描缓冲区链表，通过给定的设备号和扇区号找到对应的缓冲区）。
如果存在这样一个缓冲区，并且它还不是处于 B_BUSY 状态，bget 就会设置它的 B_BUSY 位并且返回。
如果找到的缓冲区已经在使用中，bget 就会睡眠等待它被释放。
当 sleep 返回的时候，bget 并不能假设这块缓冲区现在可用了，事实上，sleep 时释放了 buf_table_lock, 醒来后重新获取了它，这就不能保证 b 仍然是可用的缓冲区：它有可能被用来缓冲另外一个扇区。bget 非常无奈，只能重新扫描一次，希望这次能够找到可用的缓冲区。





*/

#endif