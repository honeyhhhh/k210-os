#include "include/buf.h"
#include "include/disk.h"
#include "include/sdcard.h"
#include "include/fs.h"
#include "include/spinlock.h"
#include "include/sem.h"
#include "include/proc.h"
#include "include/stddef.h"
#include "include/string.h"
#include "include/file.h"

static struct entry_cache ecache; //目录项缓冲
struct dirent root;        // 根目录项
struct list d_list;
static struct fat32 fat;          // DBR扇区信息

void fat32_init()
{
    struct buf *b = bread(0, 0);
    if (strncmp((char const *)(b->data + 0x52), "FAT32", 5))
        panic("not FAT32 volume");
    memmove(&fat.bpb.byts_per_sec, b->data + 0x0B, 2);
    fat.bpb.sec_per_clus = *(b->data + 0x0D);
    fat.bpb.rsvd_sec_cnt = *(uint16_t *)(b->data + 0x0E);
    fat.bpb.fat_cnt = *(b->data + 0x10);

    fat.bpb.hidd_sec = *(uint32_t *)(b->data + 0x1C);
    fat.bpb.tot_sec = *(uint32_t *)(b->data + 0x20);
    fat.bpb.fat_sz = *(uint32_t *)(b->data + 0x24);
    fat.bpb.root_clus = *(uint32_t *)(b->data + 0x2C);

    // printk("fsinfo sec :[%p]\n", *(uint16_t *)(b->data + 0x30));

    // 计算数据区位置　，即根目录起始扇区 = 保留区大小　+ FAT表大小　* 2
    fat.first_data_sec = fat.bpb.rsvd_sec_cnt + fat.bpb.fat_cnt * fat.bpb.fat_sz;
    // 数据区扇区个数
    fat.data_sec_cnt = fat.bpb.tot_sec - fat.first_data_sec;

    // 数据区簇数
    fat.data_clus_cnt = fat.data_sec_cnt / fat.bpb.sec_per_clus;
    // 每簇字节数
    fat.byts_per_clus = fat.bpb.sec_per_clus * fat.bpb.byts_per_sec;
    brelse(b);



    // printk("[FAT32 init]byts_per_sec: %d\n", fat.bpb.byts_per_sec); //512
    // printk("[FAT32 init]root_clus: %d\n", fat.bpb.root_clus);       // 2
    // printk("[FAT32 init]sec_per_clus: %d\n", fat.bpb.sec_per_clus); // 64
    // printk("[FAT32 init]fat_cnt: %d\n", fat.bpb.fat_cnt);           // 2
    // printk("[FAT32 init]fat_sz: %d\n", fat.bpb.fat_sz);             // 7453  0x1D1D *2 = 0x747400
    // printk("[FAT32 init]first_data_sec: %d\n", fat.first_data_sec); // 15930   

    		/* 测评机
		[FAT32 init]byts_per_sec: 512
		[FAT32 init]root_clus: 2
		[FAT32 init]sec_per_clus: 1
		[FAT32 init]fat_cnt: 2
		[FAT32 init]fat_sz: 48
		[FAT32 init]first_data_sec: 128
		[FAT32 init]tot_clus: 1892
		[FAT32 init]next_clus: 4125
		*/


    if (BSIZE != fat.bpb.byts_per_sec) 
        panic("byts_per_sec != BSIZE");

    // 目录项缓冲　锁
    initlock(&ecache.lock, "ecache");

    // 根目录项　锁
    memset(&root, 0, sizeof(root));
    sem_init(&root.lock, 1 ,"entry");

    root.filename[0] = '/';
    root.filename[1] = '\0';
    root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    root.first_clus = root.cur_clus = fat.bpb.root_clus;
    root.valid = B_BUSY;

    //list_init(&d_list);
    //list_append(&d_list, &root.d_elem);
    root.d_elem.next = &root.d_elem;
    root.d_elem.prev = &root.d_elem;

    //printk("[%p]\n",&root);

    for(struct dirent *de = ecache.entries; de < ecache.entries + ENTRY_CACHE_NUM; de++) 
    {
        de->dev = 0;
        de->valid = 0;
        de->ref = 0;
        de->dirty = 0;
        de->parent = 0;
        sem_init(&de->lock, 1, "entry");
        //printk("[%p]\n", de);
        //list_append(&d_list, &de->d_elem);
        de->d_elem.next = root.d_elem.next;
        de->d_elem.prev = &root.d_elem;
        root.d_elem.next->prev = &de->d_elem;
        root.d_elem.next = &de->d_elem;
    }


    struct buf *f1 = bread(0, 1);
    uint32_t fi;
    memmove(&fi, f1->data, 4); // 小端? 0x41615252
    if (fi != 0x41615252)
        panic("wrong in fsinfo sec [%p]!\n", fi);

    memmove(&fat.fsinfo.number_of_empty_cluster, f1->data + 0x1E8, 4);
    memmove(&fat.fsinfo.next_avail_cluster, f1->data + 0x1EC, 4);
    // printk("[FAT32 init]tot_clus: %d\n", fat.fsinfo.number_of_empty_cluster);
    // printk("[FAT32 init]next_clus: %d\n", fat.fsinfo.next_avail_cluster);
    brelse(f1);

  //printk("fat32 init done\n");
}

/*  FAT 表　*/
/*
对于文件系统来说，FAT表有两个重要作用：描述簇的分配状态以及标明文件或目录的下一簇的簇号。

FAT32中每个簇的簇地址是有32bit（4个字节），即FAT32表项   大小4字节。
0号表项与1号表项被系统保留并存储特殊标志内容。
2号表项开始，每个地址对应于数据区的簇号，FAT表中的地址编号与数据区中的簇号相同。

当文件系统被创建，即格式化操作时，分配给FAT区域的空间将会被清空
在FAT1与FAT2的0号表项与1号表项写入特定值。0--0x0FFFFFF8  1--0xFFFFFFFF或0FFFFFFF
由于创建文件系统的同时也会创建根目录，也就是为根目录分配了一个簇空间，通常为2号簇，与之对应的2号FAT表项记录为2号簇，被写入一个结束标记0x0FFFFFF8。


在文件系统中新建文件时，如果新建的文件只占用一个簇，为其分配的簇对应的FAT表项将会写入结束标记。
如果新建的文件不只占用一个簇，则在其所占用的每个簇对应的FAT表项中写入为其分配的下一簇的簇号，在最后一个簇对应的FAT表象中写入结束标记。
如果该文件结束于该簇，则在它的FAT表项中记录的是一个文件结束标记，对于FAT32而言，代表文件结束的FAT表项值为0x0FFFFFFF。
可通过cur_clus >= FAT32_EOC判断是否为结束标记
当某个簇已被分配使用，则它对应的FAT表项内的表项值也就是该文件的下一个存储位置的簇号。

新建目录时，只为其分配一个簇的空间，对应的FAT表项中写入结束标记。
当目录增大超出一个簇的大小时，将会在空闲空间中继续为其分配一个簇，并在FAT表中为其建立FAT表链以描述它所占用的簇情况。

如果某个簇存在坏扇区，则整个簇会用0xFFFFFF7标记为坏簇，这个坏簇标记就记录在它所对应的FAT表项中。
*/
#define FAT32_EOC   0x0FFFFFF8
#define FAT32_FEOC  0x0FFFFFFF
#define FAT32_BAD   0x0FFFFFF7


// file->dirent(inode) -> cluster簇号
// 簇号 - FAT表项号　- 表项对应的表项值就是下一簇号

/**
 * @param   cluster   cluster number starts from 2, which means no 0 and 1
 * 给定簇号求该簇第一个扇区号
 */
static inline uint32_t first_sec_of_clus(uint32_t cluster)
{
    return ((cluster - 2) * fat.bpb.sec_per_clus) + fat.first_data_sec;
}


/**
 * For the given number of a data cluster, return the number of the sector in a FAT table.
 * @param   cluster     number of a data cluster
 * @param   fat_num     number of FAT table from 1, shouldn't be larger than bpb::fat_cnt
 * 给定簇号求对应FAT表项所在扇区号, 因为一个表项4B，所以乘以4
 * 一个扇区512B,一个表项4B
 */
static inline uint32_t fat_sec_of_clus(uint32_t cluster, uint8_t fat_num)
{
    return fat.bpb.rsvd_sec_cnt + (cluster << 2) / fat.bpb.byts_per_sec + fat.bpb.fat_sz * (fat_num - 1);
}

/**
 * For the given number of a data cluster, return the offest in the corresponding sector in a FAT table.
 * @param   cluster   number of a data cluster
 * 给定簇号求对应FAT表项所在扇区内的偏移量
 */
static inline uint32_t fat_offset_of_clus(uint32_t cluster)
{
    return (cluster << 2) % fat.bpb.byts_per_sec;
}
//　结合上面两者就可　给定起始簇号(表项)求下一个簇号(表项值)
/**
 * Read the FAT table content corresponded to the given cluster number.
 * @param   cluster     the number of cluster which you want to read its content in FAT table
 */
static uint32_t read_fat(uint32_t cluster)
{
    if (cluster >= FAT32_EOC) {
        return cluster;
    }
    if (cluster > fat.data_clus_cnt + 1) {     // because cluster number starts at 2, not 0
        return 0;
    }
    uint32_t fat_sec = fat_sec_of_clus(cluster, 1);
    // here should be a cache layer for FAT table, but not implemented yet.
    struct buf *b = bread(0, fat_sec);
    uint32_t next_clus = *(uint32_t *)(b->data + fat_offset_of_clus(cluster));
    brelse(b);
    return next_clus;
}

/**
 * Write the FAT region content corresponded to the given cluster number.
 * @param   cluster     the number of cluster to write its content in FAT table
 * @param   content     the content which should be the next cluster number of FAT end of chain flag
 */
static int write_fat(uint32_t cluster, uint32_t content)
{
    if (cluster > fat.data_clus_cnt + 1) {
        return -1;
    }
    uint32_t fat_sec = fat_sec_of_clus(cluster, 1);
    struct buf *b = bread(0, fat_sec);
    uint off = fat_offset_of_clus(cluster);
    *(uint32_t *)(b->data + off) = content;
    bwrite(b);
    brelse(b);
    return 0;
}

// 将该簇的扇区　清零
static void zero_clus(uint32_t cluster)
{
    uint32_t sec = first_sec_of_clus(cluster);
    struct buf *b;
    for (int i = 0; i < fat.bpb.sec_per_clus; i++) {
        b = bread(0, sec++);
        memset(b->data, 0, BSIZE);
        bwrite(b);
        brelse(b);
    }
}

// 找到第一个表项值为0(未分配)的表项(簇号),并分配
static uint32_t alloc_clus(uint8_t dev)
{
    // should we keep a free cluster list? instead of searching fat every time.
    struct buf *b;
    uint32_t sec = fat.bpb.rsvd_sec_cnt;
    uint32_t const ent_per_sec = fat.bpb.byts_per_sec / sizeof(uint32_t);//每扇区128表项
    for (uint32_t i = 0; i < fat.bpb.fat_sz; i++, sec++) {
        b = bread(dev, sec);
        for (uint32_t j = 0; j < ent_per_sec; j++) {
            //printk("[%d]:[%p]\n", i*128+j,((uint32_t *)(b->data))[j]);
            if (((uint32_t *)(b->data))[j] == 0) {
                ((uint32_t *)(b->data))[j] = FAT32_FEOC;
                bwrite(b);
                brelse(b);
                uint32_t clus = i * ent_per_sec + j;
                zero_clus(clus);
                return clus;
            }
        }
        brelse(b);
    }
    return 1;
    panic("no clusters");
}

// 释放簇
static void free_clus(uint32_t cluster)
{
    write_fat(cluster, 0);
}




// eread:  rw_clus(目录项dirent->cur_clus, 0, usr_dist, off % byte_per_clus, byte_per_clus - off % byte_per_clus)
// ewrite: rw_clus(目录项dirent->cur_clus, 1, usr_dist, off % byte_per_clus, byte_per_clus - off % byte_per_clus)
// emake : rw_clus(dp->cur_clus, 1, 0, (uint64_t)&de, off, sizeof(de)); 
// eupdate: 
// eremove:
// enext
static uint rw_clus(uint32_t cluster, int write, int user, uint64_t data, uint off, uint n)
{
    //printk("%d, %p, %d, %d\n", cluster, data, off, n);
    if (off + n > fat.byts_per_clus)
        panic("offset out of range");
    uint tot, m;
    struct buf *bp;
    uint sec = first_sec_of_clus(cluster) + off / fat.bpb.byts_per_sec;
    off = off % fat.bpb.byts_per_sec;

    int bad = 0;
    for (tot = 0; tot < n; tot += m, off += m, data += m, sec++) {
        bp = bread(0, sec);
        m = BSIZE - off % BSIZE;
        if (n - tot < m) {
            m = n - tot;
        }
        if (write) {
            if ((bad = either_copyin(bp->data + (off % BSIZE), user, data, m)) != -1) {
                bwrite(bp);
            }
        } else {
            //printk("mm %d\n", m);
            bad = either_copyout(user, data, bp->data + (off % BSIZE), m);
            //printk(":%s", bp->data + (off % BSIZE));
            //printk("bad %d\n",bad);
        }
        brelse(bp);
        if (bad == -1) {
            break;
        }
    }
    return tot;
}

/**
 * for the given entry, relocate the cur_clus field based on the off
 * @param   entry       modify its cur_clus field
 * @param   off         the offset from the beginning of the relative file
 * @param   alloc       whether alloc new cluster when meeting end of FAT chains
 * @return              the offset from the new cur_clus
 */
static int reloc_clus(struct dirent *entry, uint off, int alloc)
{
    int clus_num = off / fat.byts_per_clus;
    while (clus_num > entry->clus_cnt) {
        int clus = read_fat(entry->cur_clus);
        if (clus >= FAT32_EOC) {
            if (alloc) {
                clus = alloc_clus(entry->dev);
                write_fat(entry->cur_clus, clus);
            } else {
                entry->cur_clus = entry->first_clus;
                entry->clus_cnt = 0;
                return -1;
            }
        }
        entry->cur_clus = clus;
        entry->clus_cnt++;
    }
    if (clus_num < entry->clus_cnt) {
        entry->cur_clus = entry->first_clus;
        entry->clus_cnt = 0;
        while (entry->clus_cnt < clus_num) {
            entry->cur_clus = read_fat(entry->cur_clus);
            if (entry->cur_clus >= FAT32_EOC) {
                panic("reloc_clus");
            }
            entry->clus_cnt++;
        }
    }
    return off % fat.byts_per_clus;
}

/* like the original readi, but "reade" is odd, let alone "writee" */
// Caller must hold entry->lock.
int eread(struct dirent *entry, int user_dst, uint64_t dst, uint off, uint n)
{
    if (off > entry->file_size || off + n < off || (entry->attribute & ATTR_DIRECTORY)) {
        return 0;
    }
    if (off + n > entry->file_size) {
        n = entry->file_size - off;
        //printk("%d\n %d\n", entry->file_size, off);
    }

    uint tot, m;
    for (tot = 0; entry->cur_clus < FAT32_EOC && tot < n; tot += m, off += m, dst += m) {
        reloc_clus(entry, off, 0);
        m = fat.byts_per_clus - off % fat.byts_per_clus;
        //printk("%d\n",m);
        if (n - tot < m) {
            m = n - tot;
        }
        //printk("m :%d\n", m);
        if (rw_clus(entry->cur_clus, 0, user_dst, dst, off % fat.byts_per_clus, m) != m) {
            //printk("do !\n");
            break;
        }
    }
    return tot;
}

// Caller must hold entry->lock.
int ewrite(struct dirent *entry, int user_src, uint64_t src, uint off, uint n)
{
    if (off > entry->file_size || off + n < off || (uint64_t)off + n > 0xffffffff
        || (entry->attribute & ATTR_READ_ONLY)) {
        return -1;
    }
    if (entry->first_clus == 0) {   // so file_size if 0 too, which requests off == 0
        entry->cur_clus = entry->first_clus = alloc_clus(entry->dev);
        entry->clus_cnt = 0;
        entry->dirty = 1;
    }
    uint tot, m;
    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        reloc_clus(entry, off, 1);
        m = fat.byts_per_clus - off % fat.byts_per_clus;
        if (n - tot < m) {
            m = n - tot;
        }
        if (rw_clus(entry->cur_clus, 1, user_src, src, off % fat.byts_per_clus, m) != m) {
            printk("un\n");
            break;
        }
    }
    if(n > 0) {
        if(off > entry->file_size) {
            entry->file_size = off;
            entry->dirty = 1;
        }
    }
    return tot;
}

// Returns a dirent struct. If name is given, check ecache. It is difficult to cache entries
// by their whole path. But when parsing a path, we open all the directories through it, 
// which forms a linked list from the final file to the root. Thus, we use the "parent" pointer 
// to recognize whether an entry with the "name" as given is really the file we want in the right path.
// Should never get root by eget, it's easy to understand.
static struct dirent *eget(struct dirent *parent, char *name)
{
    struct dirent *ep;
    acquire(&ecache.lock);

    if (name) 
    {
        for (ep = elem2entry(struct dirent, d_elem, root.d_elem.next); ep != &root; ep = elem2entry(struct dirent, d_elem, ep->d_elem.next)) 
        {          // LRU algo
            //printk("[%d]:[%p]\n",i++,ep);
        
            if (ep->valid == 1 && ep->parent == parent
                && strncmp(ep->filename, name, FAT32_MAX_FILENAME) == 0) {
                if (ep->ref++ == 0) {
                    ep->parent->ref++;
                }
                release(&ecache.lock);
                // edup(ep->parent);
                return ep;
            }
        }
    }
    //printk("11???????\n");
    for (ep = elem2entry(struct dirent, d_elem, root.d_elem.prev); ep != &root; ep = elem2entry(struct dirent, d_elem, ep->d_elem.prev)) {              // LRU algo
        if (ep->ref == 0) {
            ep->ref = 1;
            ep->dev = parent->dev;
            ep->off = 0;
            ep->valid = 0;
            ep->dirty = 0;
            release(&ecache.lock);
            return ep;
        }
    }
    panic("eget: insufficient ecache");
    return 0;
}

// trim ' ' in the head and tail, '.' in head, and test legality
char *formatname(char *name)
{
    static char illegal[] = { '\"', '*', '/', ':', '<', '>', '?', '\\', '|', 0 };
    char *p;
    while (*name == ' ' || *name == '.') { name++; }
    for (p = name; *p; p++) {
        char c = *p;
        if (c < 0x20 || strchr(illegal, c)) {
            return 0;
        }
    }
    while (p-- > name) {
        if (*p != ' ') {
            p[1] = '\0';
            break;
        }
    }
    return name;
}

static void generate_shortname(char *shortname, char *name)
{
    static char illegal[] = { '+', ',', ';', '=', '[', ']', 0 };   // these are legal in l-n-e but not s-n-e
    int i = 0;
    char c, *p = name;
    for (int j = strlen(name) - 1; j >= 0; j--) {
        if (name[j] == '.') {
            p = name + j;
            break;
        }
    }
    while (i < CHAR_SHORT_NAME && (c = *name++)) {
        if (i == 8 && p) {
            if (p + 1 < name) { break; }            // no '.'
            else {
                name = p + 1, p = 0;
                continue;
            }
        }
        if (c == ' ') { continue; }
        if (c == '.') {
            if (name > p) {                    // last '.'
                memset(shortname + i, ' ', 8 - i);
                i = 8, p = 0;
            }
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c += 'A' - 'a';
        } else {
            if (strchr(illegal, c) != NULL) {
                c = '_';
            }
        }
        shortname[i++] = c;
    }
    while (i < CHAR_SHORT_NAME) {
        shortname[i++] = ' ';
    }
}

uint8_t cal_checksum(unsigned char* shortname)
{
    uint8_t sum = 0;
    for (int i = CHAR_SHORT_NAME; i != 0; i--) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;
    }
    return sum;
}

/**
 * Generate an on disk format entry and write to the disk. Caller must hold dp->lock
 * @param   dp          the directory
 * @param   ep          entry to write on disk
 * @param   off         offset int the dp, should be calculated via dirlookup before calling this
 */
void emake(struct dirent *dp, struct dirent *ep, uint off)
{
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("emake: not dir");
    if (off % sizeof(union dentry))
        panic("emake: not aligned");
    
    union dentry de;
    memset(&de, 0, sizeof(de));
    if (off <= 32) {
        if (off == 0) {
            strncpy(de.sne.name, ".          ", sizeof(de.sne.name));
        } else {
            strncpy(de.sne.name, "..         ", sizeof(de.sne.name));
        }
        de.sne.attr = ATTR_DIRECTORY;
        de.sne.fst_clus_hi = (uint16_t)(ep->first_clus >> 16);        // first clus high 16 bits
        de.sne.fst_clus_lo = (uint16_t)(ep->first_clus & 0xffff);       // low 16 bits
        de.sne.file_size = 0;                                       // filesize is updated in eupdate()
        off = reloc_clus(dp, off, 1);
        rw_clus(dp->cur_clus, 1, 0, (uint64_t)&de, off, sizeof(de));
    } else {
        int entcnt = (strlen(ep->filename) + CHAR_LONG_NAME - 1) / CHAR_LONG_NAME;   // count of l-n-entries, rounds up
        char shortname[CHAR_SHORT_NAME + 1];
        memset(shortname, 0, sizeof(shortname));
        generate_shortname(shortname, ep->filename);
        de.lne.checksum = cal_checksum((unsigned char *)shortname);
        de.lne.attr = ATTR_LONG_NAME;
        for (int i = entcnt; i > 0; i--) {
            if ((de.lne.order = i) == entcnt) {
                de.lne.order |= LAST_LONG_ENTRY;
            }
            char *p = ep->filename + (i - 1) * CHAR_LONG_NAME;
            uint8_t *w = (uint8_t *)de.lne.name1;
            int end = 0;
            for (int j = 1; j <= CHAR_LONG_NAME; j++) {
                if (end) {
                    *w++ = 0xff;            // on k210, unaligned reading is illegal
                    *w++ = 0xff;
                } else { 
                    if ((*w++ = *p++) == 0) {
                        end = 1;
                    }
                    *w++ = 0;
                }
                switch (j) {
                    case 5:     w = (uint8_t *)de.lne.name2; break;
                    case 11:    w = (uint8_t *)de.lne.name3; break;
                }
            }
            uint off2 = reloc_clus(dp, off, 1);
            rw_clus(dp->cur_clus, 1, 0, (uint64_t)&de, off2, sizeof(de));
            off += sizeof(de);
        }
        memset(&de, 0, sizeof(de));
        strncpy(de.sne.name, shortname, sizeof(de.sne.name));
        de.sne.attr = ep->attribute;
        de.sne.fst_clus_hi = (uint16_t)(ep->first_clus >> 16);      // first clus high 16 bits
        de.sne.fst_clus_lo = (uint16_t)(ep->first_clus & 0xffff);     // low 16 bits
        de.sne.file_size = ep->file_size;                         // filesize is updated in eupdate()
        off = reloc_clus(dp, off, 1);
        rw_clus(dp->cur_clus, 1, 0, (uint64_t)&de, off, sizeof(de));
    }
}

/**
 * Allocate an entry on disk. Caller must hold dp->lock.
 */
struct dirent *ealloc(struct dirent *dp, char *name, int attr)
{
    if (!(dp->attribute & ATTR_DIRECTORY)) {
        panic("ealloc not dir");
    }
    if (dp->valid != 1 || !(name = formatname(name))) {        // detect illegal character
        return NULL;
    }
    struct dirent *ep;
    uint off = 0;
    if ((ep = dirlookup(dp, name, &off)) != 0) {      // entry exists
        return ep;
    }
    ep = eget(dp, name);
    elock(ep);
    ep->attribute = attr;
    ep->file_size = 0;
    ep->first_clus = 0;
    ep->parent = edup(dp); // 自己作为自己的父母，ref++
    ep->off = off;
    ep->clus_cnt = 0;
    ep->cur_clus = 0;
    ep->dirty = 0;
    strncpy(ep->filename, name, FAT32_MAX_FILENAME);
    ep->filename[FAT32_MAX_FILENAME] = '\0';
    if (attr == ATTR_DIRECTORY) {    // generate "." and ".." for ep
        ep->attribute |= ATTR_DIRECTORY;
        ep->cur_clus = ep->first_clus = alloc_clus(dp->dev);
        emake(ep, ep, 0);
        emake(ep, dp, 32);
    } else {
        ep->attribute |= ATTR_ARCHIVE;
    }
    emake(dp, ep, off);
    ep->valid = 1;
    eunlock(ep);
    return ep;
}

struct dirent *edup(struct dirent *entry)
{
    if (entry != 0) {
        acquire(&ecache.lock);
        entry->ref++;
        release(&ecache.lock);
    }
    return entry;
}

// Only update filesize and first cluster in this case.
// caller must hold entry->parent->lock
void eupdate(struct dirent *entry)
{
    if (!entry->dirty || entry->valid != 1) { return; }
    uint entcnt = 0;
    uint32_t off = reloc_clus(entry->parent, entry->off, 0);
    rw_clus(entry->parent->cur_clus, 0, 0, (uint64_t) &entcnt, off, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    off = reloc_clus(entry->parent, entry->off + (entcnt << 5), 0);
    union dentry de;
    rw_clus(entry->parent->cur_clus, 0, 0, (uint64_t)&de, off, sizeof(de));
    de.sne.fst_clus_hi = (uint16_t)(entry->first_clus >> 16);
    de.sne.fst_clus_lo = (uint16_t)(entry->first_clus & 0xffff);
    de.sne.file_size = entry->file_size;
    rw_clus(entry->parent->cur_clus, 1, 0, (uint64_t)&de, off, sizeof(de));
    entry->dirty = 0;
}

// caller must hold entry->lock
// caller must hold entry->parent->lock
// remove the entry in its parent directory
void eremove(struct dirent *entry)
{
    if (entry->valid != 1) { return; }
    uint entcnt = 0;
    uint32_t off = entry->off;
    uint32_t off2 = reloc_clus(entry->parent, off, 0);
    rw_clus(entry->parent->cur_clus, 0, 0, (uint64_t) &entcnt, off2, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    uint8_t flag = EMPTY_ENTRY;
    for (int i = 0; i <= entcnt; i++) {
        rw_clus(entry->parent->cur_clus, 1, 0, (uint64_t) &flag, off2, 1);
        off += 32;
        off2 = reloc_clus(entry->parent, off, 0);
    }
    entry->valid = -1;
}

// truncate a file
// caller must hold entry->lock
void etrunc(struct dirent *entry)
{
    for (uint32_t clus = entry->first_clus; clus >= 2 && clus < FAT32_EOC; ) {
        uint32_t next = read_fat(clus);
        free_clus(clus);
        clus = next;
    }
    entry->file_size = 0;
    entry->first_clus = 0;
    entry->dirty = 1;
}

void elock(struct dirent *entry)
{
    if (entry == 0 || entry->ref < 1)
        panic("elock");
    sem_wait(&entry->lock);
}

void eunlock(struct dirent *entry)
{
    if (entry == 0 || !sem_hold(&entry->lock) || entry->ref < 1)
        panic("eunlock");
    sem_signal(&entry->lock);
}

void eput(struct dirent *entry)
{
    acquire(&ecache.lock);
    if (entry != &root && entry->valid != 0 && entry->ref == 1) {
        // ref == 1 means no other process can have entry locked,
        // so this acquiresleep() won't block (or deadlock).
        sem_wait(&entry->lock);
        // entry->next->prev = entry->prev;
        // entry->prev->next = entry->next;
        list_remove(&entry->d_elem);
        // 将链表元素插在root之后
        entry->d_elem.next = root.d_elem.next;
        entry->d_elem.prev = &root.d_elem;
        root.d_elem.next->prev = &entry->d_elem;
        root.d_elem.next = &entry->d_elem;
        
        release(&ecache.lock);
        if (entry->valid == -1) {       // this means some one has called eremove()
            etrunc(entry);
        } else {
            elock(entry->parent);
            eupdate(entry);
            eunlock(entry->parent);
        }
        sem_signal(&entry->lock);

        // Once entry->ref decreases down to 0, we can't guarantee the entry->parent field remains unchanged.
        // Because eget() may take the entry away and write it.
        struct dirent *eparent = entry->parent;
        acquire(&ecache.lock);
        entry->ref--;
        release(&ecache.lock);
        if (entry->ref == 0) {
            eput(eparent);
        }
        return;
    }
    entry->ref--;
    release(&ecache.lock);
}

void estat(struct dirent *de, struct stat *st)
{
    strncpy(st->name, de->filename, STAT_MAX_NAME);
    st->type = (de->attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    st->dev = de->dev;
    st->size = de->file_size;
}

/**
 * Read filename from directory entry.
 * @param   buffer      pointer to the array that stores the name
 * @param   raw_entry   pointer to the entry in a sector buffer
 * @param   islong      if non-zero, read as l-n-e, otherwise s-n-e.
 */
static void read_entry_name(char *buffer, union dentry *d)
{
    if (d->lne.attr == ATTR_LONG_NAME) {                       // long entry branch
        uint16_t temp[NELEM(d->lne.name1)];
        memmove(temp, d->lne.name1, sizeof(temp));
        snstr(buffer, temp, NELEM(d->lne.name1));
        buffer += NELEM(d->lne.name1);
        snstr(buffer, d->lne.name2, NELEM(d->lne.name2));
        buffer += NELEM(d->lne.name2);
        snstr(buffer, d->lne.name3, NELEM(d->lne.name3));
    } else {
        // assert: only "." and ".." will enter this branch
        memset(buffer, 0, CHAR_SHORT_NAME + 2); // plus '.' and '\0'
        int i;
        for (i = 0; d->sne.name[i] != ' ' && i < 8; i++) {
            buffer[i] = d->sne.name[i];
        }
        if (d->sne.name[8] != ' ') {
            buffer[i++] = '.';
        }
        for (int j = 8; j < CHAR_SHORT_NAME; j++, i++) {
            if (d->sne.name[j] == ' ') { break; }
            buffer[i] = d->sne.name[j];
        }
    }
}


void print_all_file_name()
{
    char name[255];
    union dentry de;
    rw_clus(2, 0, 0, (uint64_t)&de, 64, 32);// != 32 || de.lne.order == END_OF_ENTRY);
    read_entry_name(name, &de);
  //printk("%s\n", name);

}


/**
 * Read entry_info from directory entry.
 * @param   entry       pointer to the structure that stores the entry info
 * @param   raw_entry   pointer to the entry in a sector buffer
 */
static void read_entry_info(struct dirent *entry, union dentry *d)
{
    entry->attribute = d->sne.attr;
    entry->first_clus = ((uint32_t)d->sne.fst_clus_hi << 16) | d->sne.fst_clus_lo;
    entry->file_size = d->sne.file_size;
    entry->cur_clus = entry->first_clus;
    entry->clus_cnt = 0;
}

/**
 * Read a directory from off, parse the next entry(ies) associated with one file, or find empty entry slots.
 * Caller must hold dp->lock.
 * @param   dp      the directory
 * @param   ep      the struct to be written with info
 * @param   off     offset off the directory
 * @param   count   to write the count of entries
 * @return  -1      meet the end of dir
 *          0       find empty slots
 *          1       find a file with all its entries
 */
int enext(struct dirent *dp, struct dirent *ep, uint off, int *count)
{
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("enext not dir");
    if (ep->valid)
        panic("enext ep valid");
    if (off % 32)
        panic("enext not align");
    if (dp->valid != 1) { return -1; }

    union dentry de;
    int cnt = 0;
    memset(ep->filename, 0, FAT32_MAX_FILENAME + 1);
    for (int off2; (off2 = reloc_clus(dp, off, 0)) != -1; off += 32) {
        if (rw_clus(dp->cur_clus, 0, 0, (uint64_t)&de, off2, 32) != 32 || de.lne.order == END_OF_ENTRY) {
            return -1;
        }
        if (de.lne.order == EMPTY_ENTRY) {
            cnt++;
            continue;
        } else if (cnt) {
            *count = cnt;
            return 0;
        }
        if (de.lne.attr == ATTR_LONG_NAME) {
            int lcnt = de.lne.order & ~LAST_LONG_ENTRY;
            if (de.lne.order & LAST_LONG_ENTRY) {
                *count = lcnt + 1;                              // plus the s-n-e;
                count = 0;
            }
            read_entry_name(ep->filename + (lcnt - 1) * CHAR_LONG_NAME, &de);
        } else {
            if (count) {
                *count = 1;
                read_entry_name(ep->filename, &de);
            }
            read_entry_info(ep, &de);
            return 1;
        }
    }
    return -1;
}

/**
 * Seacher for the entry in a directory and return a structure. Besides, record the offset of
 * some continuous empty slots that can fit the length of filename.
 * Caller must hold entry->lock.
 * @param   dp          entry of a directory file
 * @param   filename    target filename
 * @param   poff        offset of proper empty entry slots from the beginning of the dir
 */
struct dirent *dirlookup(struct dirent *dp, char *filename, uint *poff)
{
    //printk("??");
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("dirlookup not DIR");
    if (strncmp(filename, ".", FAT32_MAX_FILENAME) == 0) {
        return edup(dp);
    } else if (strncmp(filename, "..", FAT32_MAX_FILENAME) == 0) {
        if (dp == &root) {
            return edup(&root);
        }
        return edup(dp->parent);
    }
    if (dp->valid != 1) {
        return NULL;
    }
    struct dirent *ep = eget(dp, filename);
    //printk("[%p]\n", ep);
    if (ep->valid == 1) { return ep; }                               // ecache hits

    int len = strlen(filename);
    int entcnt = (len + CHAR_LONG_NAME - 1) / CHAR_LONG_NAME + 1;   // count of l-n-entries, rounds up. plus s-n-e
    int count = 0;
    int type;
    uint off = 0;
    reloc_clus(dp, 0, 0);
    while ((type = enext(dp, ep, off, &count) != -1)) {
        if (type == 0) {
            if (poff && count >= entcnt) {
                *poff = off;
                poff = 0;
            }
        } else if (strncmp(filename, ep->filename, FAT32_MAX_FILENAME) == 0) {
            ep->parent = edup(dp);
            ep->off = off;
            ep->valid = 1;
            return ep;
        }
        off += count << 5;
    }
    if (poff) {
        *poff = off;
    }
    eput(ep);
    return NULL;
}

static char *skipelem(char *path, char *name)
{
    while (*path == '/') {
        path++;
    }
    if (*path == 0) { return NULL; }
    char *s = path;
    while (*path != '/' && *path != 0) {
        path++;
    }
    int len = path - s;
    //printk("len %d\n", len);
    if (len > FAT32_MAX_FILENAME) {
        len = FAT32_MAX_FILENAME;
    }
    name[len] = 0;
    memmove(name, s, len);
    while (*path == '/') {
        path++;
    }
    return path;
}

// FAT32 version of namex in xv6's original file system.
static struct dirent *lookup_path(char *path, int parent, char *name)
{
    struct dirent *entry, *next;
    if (*path == '/') {
        entry = edup(&root);
    } else if (*path != '\0') {
        entry = edup(mytask()->cwd);
    } else {
        return NULL;
    }
    while ((path = skipelem(path, name)) != 0) 
    {
    //     printk("path :%s\n",path);
    //   printk("look for name :[%s]\n", name);
    //   printk("now in dir [%s]\n", entry->filename);
        elock(entry);
        if (!(entry->attribute & ATTR_DIRECTORY)) {
            eunlock(entry);
            eput(entry);
            return NULL;
        }
        if (parent && *path == '\0') {
            eunlock(entry);
            return entry;
        }
        if ((next = dirlookup(entry, name, 0)) == 0) {
          //printk("no found [%s] in dir [%s]\n", name, entry->filename);
            eunlock(entry);
            eput(entry);
            return NULL;
        }
      //printk("found [%s] in dir [%s]\n", name, entry->filename);

        eunlock(entry);
        eput(entry);
        entry = next;
    }
    if (parent) {
        eput(entry);
        return NULL;
    }
    //printk("return !\n");
    return entry;
}

struct dirent *ename(char *path)
{
    char name[FAT32_MAX_FILENAME + 1];
    //printk("look fpr [%s]\n", path);
    return lookup_path(path, 0, name);
}

struct dirent *enameparent(char *path, char *name)
{
    return lookup_path(path, 1, name);
}


void file_search(char *path)
{
    char name[16][255];
    int sp = 0;

    char t[255];


    strncpy(name[sp++], path, 255);


    struct dirent *ep;

    
    struct dirent *ep2;
 

    //int len = 0;
    //int entcnt = (len + CHAR_LONG_NAME - 1) / CHAR_LONG_NAME + 1;   // count of l-n-entries, rounds up. plus s-n-e
    while (sp != 0)
    {
        memset(t, 0, sizeof(t));
        sp--;
        //printk("name[%d]:[%s]  len:[%d]\n",sp, name[sp], strlen(name[sp]));
        strncpy(t, name[sp], strlen(name[sp])+1);
        //memset(name[sp], 0, sizeof(name[sp]));
        //printk("now search dir :%s\n",t);
        ep = ename(t);

        if (ep)
        {
            //printk("type :%p\n", ep->attribute);
            //printk("cluster :[%d]\n",ep->first_clus);
        }
        if (!(ep->attribute & ATTR_DIRECTORY))
            panic("dirlookup not DIR");

        ep2 = eget(ep, NULL);
        if (ep2->valid == 1) { panic("unknow wrong"); }                               // ecache hits


        int count = 0;
        int type;
        uint off = 0;
        reloc_clus(ep, 0, 0);
        while ((type = enext(ep, ep2, off, &count)) != -1)
        {
            printk("found %d: %s\nfile_size :[%d]\nfirst_cluster:[%d]\n",ep2->attribute, ep2->filename, ep2->file_size, ep2->first_clus);
            
            // test strncmp(ep2->filename, "busybox", FAT32_MAX_FILENAME) == 0
            if (0)
            {
                //printk("\nfound %d: %s\nfile_size :[%d]B\nfirst_cluster:[%d]\n\n",ep2->attribute, ep2->filename, ep2->file_size, ep2->first_clus);
                //int r;
                //printk("[%d]\n",buddy_remain_size(HEAP_ALLOCATOR));
                uint64_t addr = (uint64_t)buddy_alloc(HEAP_ALLOCATOR, ep2->file_size); //64k
                if (addr == 0)
                    panic("alloc failed!\n");
                eread(ep2, 0, addr, 0, ep2->file_size);
                // printk("read :[%d]\n", r);
                // printk("clus cnt [%d]\ncur [%d]\n", ep2->clus_cnt, ep2->cur_clus);
                printk("memory remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));
    uint64_t page_nums = FRAME_ALLOCATOR.end - FRAME_ALLOCATOR.current;
    printk("remain %d (%p ~ %p) Phys Frames \n", page_nums, FRAME_ALLOCATOR.current, FRAME_ALLOCATOR.end);
                uint32_t *elf_mag = (uint32_t *)(addr);
                if (*elf_mag == 0x464c457f)
                {
                    struct TaskControlBlock *t = tcb_new((uint8_t *)addr, ep2->file_size, ep);
                    //printk("filename :[%s] pid :[%d]\n", ep2->filename, t->pid);
                    task_create(t);
                }

                buddy_free(HEAP_ALLOCATOR, (void *)addr);
                printk("memory remain size :[%d] byte\n", buddy_remain_size(HEAP_ALLOCATOR));


            }









            if (ep2->attribute & ATTR_DIRECTORY && ep2->filename[0] != '.')
            {
                // t = name[sp] = "/\0"
                // ep2->filename = "riscv64\0"
                strcat(name[sp], ep2->filename);
                strcat(name[sp], "/");
                sp++;
            }
            off += count << 5;
        }
        eput(ep);
    }


}





int ls(struct file *f, uint64_t addr, int size)
{

    struct dirent *ep = f->ep;

    
    struct dirent ep2;
    struct linux_dirent64 dt;
    struct linux_dirent64 *dt2 = NULL;;

    if (!(ep->attribute & ATTR_DIRECTORY) || f == NULL)
        return -1;


    int count = 0;
    int type;
    uint off = 0;
    reloc_clus(ep, 0, 0);
    int nread = 0;
    //printk("sizeof linux dent :[%d]\n", sizeof(dt));
    //printk("dt [%p] [%p] [%p] [%p] [%p] [%p]\n",&dt, &dt.d_ino, &dt.d_off, &dt.d_reclen, &dt.d_type ,&dt.d_name);

    while ((type = enext(ep, &ep2, off, &count)) != -1)
    {
        //printk("file :[%s]   [%d]\n", ep2.filename, strlen(ep2.filename));
        
        //strncpy(dt.d_name, ep2.filename, strlen(ep2.filename)+1);
        //printk("sizeof linux dent :[%d]\n", sizeof(dt)); 24

        if (nread + strlen(ep2.filename) + 1 + 19 >= size)
            return nread;

        dt.d_ino = ep2.first_clus;
        dt.d_type = ep2.attribute;
        dt.d_reclen = 19 + strlen(ep2.filename) + 1;
        dt.d_off = addr + dt.d_reclen;


        dt2 = (struct linux_dirent64 *)addr;
        copyout2((uint64_t)dt2->d_name, ep2.filename, strlen(ep2.filename)+1);
        copyout2(addr, (char *)&dt, 19);


        addr += dt.d_reclen;
        nread += dt.d_reclen;


        off += count << 5;


    }
    

    return nread;
}


















































void fat_test()
{
    struct buf *b;
    uint32_t sec = fat.bpb.rsvd_sec_cnt;
    uint32_t const ent_per_sec = fat.bpb.byts_per_sec / sizeof(uint32_t);//每扇区128表项
    for (uint32_t i = 0; i < 2; i++, sec++) {
        b = bread(0, sec);
        for (uint32_t j = 0; j < ent_per_sec; j++) {
          //printk("[%d]:[%p]\n", i*128+j,((uint32_t *)(b->data))[j]);
        }
        brelse(b);
    }


    // printk("[FAT32 init]next_clus: %d\n", fat.fsinfo.next_avail_cluster);
    // uint32_t clus = alloc_clus(0);
    // printk("next [%d]\n", clus);

    // uint32_t i;
    // i = read_fat(fat.fsinfo.next_avail_cluster);
    // printk("[%p]\n", i);

    // free_clus(fat.fsinfo.next_avail_cluster);


    // uint8_t f1[BSIZE];
    // sdcard_read_sector(f1, 1+0x800);
    // memmove(&fat.fsinfo.next_avail_cluster, f1 + 0x1EC, 4);
    // printk("[FAT32 init]next_clus: %d\n", fat.fsinfo.next_avail_cluster);


    // i = read_fat(fat.fsinfo.next_avail_cluster);
    // printk("[%p]\n", i);

}


