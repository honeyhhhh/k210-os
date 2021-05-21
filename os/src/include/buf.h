#ifndef __BUF_H
#define __BUF_H

#define BSIZE 512

#define NBUCKET 13
#define NBUF (NBUCKET * 3)

// #include "sleeplock.h"
#include "spinlock.h"

struct buf {
    int valid;
    int disk;		// does disk "own" buf? 
    uint32_t dev;
    uint32_t sectorno;	// sector number 
    struct spinlock lock;
    uint32_t refcnt;
    struct buf *prev;
    struct buf *next;
    uint8_t data[BSIZE];
};

void            binit(void);
struct buf*     bread(uint32_t, uint32_t);
void            brelse(struct buf *r);
void            bwrite(struct buf *w);

#endif