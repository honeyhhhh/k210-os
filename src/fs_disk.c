#include "include/buf.h"
#include "include/dmac.h"
#include "include/sdcard.h"
#include "include/string.h"
#include "include/stdio.h"

uint32_t DBRsec = 0;

void disk_init(void)
{
    sdcard_init();

    // 判断是否有MBR区
    uint8_t buf1[BSIZE];
    sdcard_read_sector(buf1, 1);
    uint32_t fi;

    // 标志　52526141
    //memmove(&fi, buf1, 4); // 小端? 0x41615252
    //fi = *((uint16_t *)buf1); // 0x5252
    memmove(&fi, buf1, 2);
    //panic("%p\n", fi);
    if (fi != 0x5252)
    {
        uint8_t buf2[BSIZE];
        sdcard_read_sector(buf2, 0);
        memmove(&DBRsec, buf2 + 0x1C6, 4);
    }
    else
    {
        //printk("no found MBR sec\n");
    }
    



}


void disk_read(struct buf *r)
{
    //printk("read [%p] + [%p]\n", r->sectorno, DBRsec);
    sdcard_read_sector(r->data, r->sectorno + DBRsec);
}


void disk_write(struct buf *w)
{
    sdcard_write_sector(w->data, w->sectorno + DBRsec);
}


void dist_intr(void)
{
    
}