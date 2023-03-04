#include "include/buf.h"
#include "include/disk.h"
#include "include/buf.h"
#include "include/spinlock.h"
#include "include/sem.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/assert.h"


struct {
    struct spinlock lock;
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    struct buf head;
} bcache;

void binit(void)
{
    struct buf *b;

    initlock(&bcache.lock, "bcache");

    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for(b = bcache.buf; b < bcache.buf+NBUF; b++)
    {
        b->refcnt = 0;
        b->sectorno = ~0;
        b->dev = ~0;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        sem_init(&b->lock, 1 ,"buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
    //printk("binit\n");

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf* bget(uint dev, uint sectorno)
{
    struct buf *b;

    acquire(&bcache.lock);

    // Is the block already cached?
    for(b = bcache.head.next; b != &bcache.head; b = b->next)
    {
        //printk("b->dev :[%p] dev :[%p] \n", b->dev, dev);
        if(b->dev == dev && b->sectorno == sectorno)
        {
            b->refcnt++;
            release(&bcache.lock);
            sem_wait(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for(b = bcache.head.prev; b != &bcache.head; b = b->prev)
    {
        if(b->refcnt == 0) 
        {
            b->dev = dev;
            b->sectorno = sectorno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            sem_wait(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
    return NULL;
}

// Return a locked buf with the contents of the indicated block.
struct buf* bread(uint dev, uint sectorno) 
{
    struct buf *b;

    b = bget(dev, sectorno);
    if (!b->valid) 
    {
        disk_read(b);
        b->valid = B_BUSY;
    }

    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) 
{
    if(!sem_hold(&b->lock))
        panic("bwrite");
    disk_write(b);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
    if(!sem_hold(&b->lock))
        panic("brelse");

    sem_signal(&b->lock);
    acquire(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
    release(&bcache.lock);
}

void bpin(struct buf *b) 
{
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void bunpin(struct buf *b) 
{
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}


void buf_test()
{
    // printk("MBR sec: \n");
    // struct buf *mbr = bread(0, 0);
    // for (int i = 0; i < BSIZE; i++) 
    // {
	//     if (0 == i % 16) {
	// 		printk("\n");
	// 	}

	// 	printk("%x\t", mbr->data[i]);
	// }
    // printk("\n");
    // printk("FSINFO sec: \n");
    // struct buf *fsinfo = bread(0, 1);
    // for (int i = 0; i < BSIZE; i++) 
    // {
	//     if (0 == i % 16) {
	// 		printk("\n");
	// 	}

	// 	printk("%x\t", fsinfo->data[i]);
	// }
    // printk("\n");
    // printk("FSINFO sec: \n");
    // struct buf *fsinfo = bread(0, 1);
    // for (int i = 0; i < BSIZE; i++) 
    // {
	//     if (0 == i % 16) {
	// 		printk("\n");
	// 	}

	// 	printk("%x\t", fsinfo->data[i]);
	// }
    // printk("\n");

    // uint32_t DBRsec = *((uint32_t *)(mbr->data + 0x1C6));
    // [rustsbi] panicked at 'unhandled trap! mcause: Exception(LoadMisaligned)
    // uint32_t DBRsec;
    // memmove(&DBRsec, mbr->data + 0x1C6, 4);

    //printk("DBR sec: [%p]\n", 0);
    // printk("DBR section:\n");
    struct buf *dbr = bread(0, 0);
    // for (int i = 0; i < BSIZE; i++) 
    // {
	//     if (0 == i % 16) {
	// 		printk("\n");
	// 	}

	// 	printk("%x\t", dbr->data[i]);
	// }
    // printk("\n");

    if (!strncmp((char const *)(dbr->data + 0x52), "FAT32", 5))
        printk("found FAT32!\n");
    

}

