#include "include/buf.h"
#include "include/dmac.h"
#include "include/sdcard.h"



void disk_init(void)
{
    sdcard_init();
}


void disk_read(struct buf *r)
{
    sdcard_read_sector(r->data, r->sectorno);
}


void disk_write(struct buf *w)
{
    sdcard_write_sector(w->data, w->sectorno);
}


void dist_intr(void)
{
    
}