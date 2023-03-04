#ifndef _FS_H
#define _FS_H

#include "stddef.h"
#include "sem.h"
#include "file.h"

/* 缓冲 */
#define BSIZE 512
#define NBUCKET 13
#define NBUF (NBUCKET * 3)
#define B_BUSY 0x1  // buffer is locked by some process
#define B_VALID 0x2 // buffer has been read from disk
#define B_DIRTY 0x4 // buffer needs to be written to disk

/* 操作权限 */
#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002
#define O_APPEND 0x004
// #define O_CREATE 0x200
#define O_CREATE 0x40
#define O_TRUNC 0x400
#define O_DIRECTORY 0x0200000

// typedef struct
// {
//     uint64_t dev;    // 文件所在磁盘驱动器号，不考虑
//     uint64_t ino;    // inode 文件所在 inode 编号
//     uint32_t mode;   // 文件类型
//     uint32_t nlink;  // 硬链接数量，初始为1
//     uint64_t pad[7]; // 无需考虑，为了兼容性设计
// } stat;

/* 文件类型 */
#define T_DIR 1  // Directory
#define T_FILE 2 // File
#define T_DEV 3  // Device

#define STAT_MAX_NAME 32
/* 文件或目录属性数据结构 */
struct stat
{
    char name[STAT_MAX_NAME + 1];
    int dev;       // File system's disk device
    short type;    // Type of file
    uint64_t size; // Size of file in bytes
};

/*－－－－　 FAT32　－－－－－－ */

// DBR 区  0号扇区
/*
0x00~0x02：3字节，跳转指令。
0x03~0x0A：8字节，文件系统标志和版本号，这里为mkfs.fat
0x0B~0x0C：2字节，每扇区字节数，0x0200=512
0x0D~0x0D：1字节，每簇扇区数，0x01
0x0E~0x0F：2字节，保留扇区数，决定FAT区的位置
0x10~0x10：1字节，FAT表个数，0x02
0x11~0x12：2字节，FAT32必须等于0，FAT12/FAT16为根目录中目录的个数；
0x13~0x14：2字节，FAT32必须等于0，FAT12/FAT16为扇区总数。
0x15~0x15：1字节，哪种存储介质，0xF8标准值，可移动存储介质。
0x16~0x17：2字节，FAT32必须为0，FAT12/FAT16为一个FAT 表所占的扇区数。
0x18~0x19：2字节，每磁道扇区数，只对于“特殊形状”（由磁头和柱面分割为若干磁道）的存储介质有效，0x003F=63。
0x1A~0x1B：2字节，磁头数，只对特殊的介质才有效，
0x1C~0x1F：4字节，EBR分区之前所隐藏的扇区数，与MBR中地址0x1C6开始的4个字节数值相等。
0x20~0x23：4字节，文件系统总扇区数，
0x24~0x27：4字节，每个FAT表占用扇区数，决定数据区位置
0x28~0x29：2字节，标记，此域FAT32 特有。
0x2A~0x2B：2字节，FAT32版本号0.0，FAT32特有。
0x2C~0x2F：4字节，根目录所在第一个簇的簇号，0x02。（虽然在FAT32文件系统下，根目录可以存放在数据区的任何位置，但是通常情况下还是起始于2号簇）
0x30~0x31：2字节，FSINFO（文件系统信息扇区）扇区号0x01，该扇区为操作系统提供关于空簇总数及下一可用簇的信息。
0x32~0x33：2字节，备份引导扇区的位置。备份引导扇区总是位于文件系统的6号扇区。
0x34~0x3F：12字节，用于以后FAT 扩展使用。
0x40~0x40：1字节，与FAT12/16 的定义相同，只不过两者位于启动扇区不同的位置而已。
0x41~0x41：1字节，与FAT12/16 的定义相同，只不过两者位于启动扇区不同的位置而已 。
0x42~0x42：1字节，扩展引导标志，0x29。与FAT12/16 的定义相同，只不过两者位于启动扇区不同的位置而已
0x43~0x46：4字节，卷序列号。通常为一个随机值。
0x47~0x51：11字节，卷标（ASCII码），如果建立文件系统的时候指定了卷标，会保存在此。
0x52~0x59：8字节，文件系统格式的ASCII码，FAT32。
0x5A~0x1FD：共420字节，引导代码。
0x1FE~0x1FF：签名标志“55 AA”。 
*/
struct fat32
{
    uint32_t first_data_sec; // 由rsvd_sec_cnt和　fat_size　共同决定
    uint32_t data_sec_cnt;
    uint32_t data_clus_cnt;
    uint32_t byts_per_clus;

    struct
    {
        uint16_t byts_per_sec; // 0x0B
        uint8_t sec_per_clus;  // 0x0D
        uint16_t rsvd_sec_cnt; // 0x0E                            ----决定FAT位置-----
        uint8_t fat_cnt;       /* count of FAT regions               FAT表个数=2       0x10*/
        uint32_t hidd_sec;     /* count of hidden sectors               0x1C 测评机没有MBR扇区 */
        uint32_t tot_sec;      /* total count of sectors including all regions   0x20 文件系统总扇区数 */
        uint32_t fat_sz;       /* count of sectors for a FAT region              0x24 每个FAT表占用扇区数，----决定数据区位置--- */
        uint32_t root_clus;    // 0x2C 根目录所在一个簇簇号　一般是0x02
    } bpb;

    struct
    {
        uint32_t number_of_empty_cluster;
        uint32_t next_avail_cluster;
    } fsinfo;
};

// FSINFO区  1号扇区
// 用以记录文件系统中空闲簇的数量以及下一可用簇的簇号等信息

/*
0x3E8~0x3EB：4个字节，文件系统的空簇数，
0x3EC~0x3EF：4个字节，下一可用簇号。
*/

// FAT32 的根目录　位于数据区，　紧跟FAT2区后
//　根目录位于数据区起始扇区
// 根目录由若干个目录项组成，一个目录项占用32个字节，可以是长文件名目录项、文件目录项、“.”目录项和“..”目录项等

// 短文件名目录项
/*
    0x00-0x07：文件名，不足8个字节0x20补全()
    0x08-0x0A：扩展名
    0x0B：文件属性，0x20表示归档，0x01只读，0x02隐藏，0x04系统，0x08卷标。0x10子目录，0x0F长文件
    0x0C: 保留
    0x0D：创建时间的10毫秒位
    0x0E-0x0F：文件创建时间
    0x10-0x11：文件创建日期
    0x12-0x13：文件最后访问日期
    0x14-0x15：文件起始簇号的高16位 0x0000
    0x16-0x17：文件最近修改时间
    0x18-0x19：文件最近修改日期
    0x1A-0x1B：文件起始簇号的低16位 0x0003 （FAT表项号）
    0x1C-0x1F：文件的长度  0xF8A8 = 63656  ≈  64KB
*/
// 长文件名目录项, 文件名大于8和后缀名大于3的文件
/*
    0x0: 属性字节位意义
    0x01-0x0A: 长文件名unicode, 每个字符２字节
    0x0B: 长文件名目录项标志　0x0F
    0x0C: 保留
    0x0D: 校验值
    0x0E-0x19: 长文件名unicode
    0x1A-0x1B: 文件起始簇号
    0x1C-0x1D: 长文件名unicode
*/
// 文件名+扩展名
#define CHAR_SHORT_NAME 11
#define CHAR_LONG_NAME 13

// 短0x0B　文件属性
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
//　长0x0B 文件属性
#define ATTR_LONG_NAME 0x0F
// 第6位为1表示　这是长目录文件最后一个目录项
#define LAST_LONG_ENTRY 0x40

// 目录项的第一个字节为“0xE5”，表示该项已被删除。
#define EMPTY_ENTRY 0xe5
// 目录项的第一个字节为0x2E(“.”)，表示当前目录的信息（目录的数据空间也会保存当前目录的目录项信息）
// 目录项的前两个字节为“0x2E 0x2E(“. .”)，表示上一级目录。
// 目录项的第一个字节为"0x00"，代表从此位置开始以后的数据空间都没有使用。
#define END_OF_ENTRY 0x00

typedef struct short_name_entry
{
    char name[CHAR_SHORT_NAME];
    uint8_t attr;
    uint8_t _nt_res;
    uint8_t _crt_time_tenth;
    uint16_t _crt_time;
    uint16_t _crt_date;
    uint16_t _lst_acce_date;
    uint16_t fst_clus_hi;
    uint16_t _lst_wrt_time;
    uint16_t _lst_wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} __attribute__((packed, aligned(4))) short_name_entry_t;

typedef struct long_name_entry
{
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t _type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t _fst_clus_lo;
    uint16_t name3[2];
} __attribute__((packed, aligned(4))) long_name_entry_t;

// 目录项数据结构
union dentry
{
    short_name_entry_t sne;
    long_name_entry_t lne;
};

// 上述结构的有些字段并没有使用
// FAT32 没有磁盘inode　所以 替换为　目录/文件　数据结构 dirent
#define FAT32_MAX_FILENAME 255
#define FAT32_MAX_PATH 260
struct dirent
{
    char filename[FAT32_MAX_FILENAME + 1];
    uint8_t attribute;
    // uint8_t   create_time_tenth;
    // uint16_t  create_time;
    // uint16_t  create_date;
    // uint16_t  last_access_date;
    uint32_t first_clus; // 16+16
    // uint16_t  last_write_time;
    // uint16_t  last_write_date;
    uint32_t file_size;

    uint32_t cur_clus;
    uint32_t clus_cnt;

    /* for OS */
    uint8_t dev;
    uint8_t dirty;
    short valid;
    int ref;
    uint32_t off;          // offset in the parent dir entry, for writing convenience
    struct dirent *parent; // because FAT32 doesn't have such thing like inum, use this for cache trick

    struct list_elem d_elem;

    struct semaphore lock;
};
extern struct dirent root;

// 目录项缓冲
#define ENTRY_CACHE_NUM 50
struct entry_cache
{
    struct spinlock lock;
    struct dirent entries[ENTRY_CACHE_NUM];
};

// 主要是对目录项内容的操作, 需要在多进程下同步

void fat32_init();

// test
void fat_test();
void print_all_file_name();
void file_search(char *path);
int ls(struct file *f, uint64_t addr, int size);


// 1 disk
// 2 buf

// 4 Inode/entry Layer
// 分配目录项　一开始磁盘上的 entry 与内存中的 dirent 只是简单的建立了映射关系，它们的内容并没有联系 需要进行如下步骤
struct dirent *ealloc(struct dirent *dp, char *name, int attr);
// 1 static struct dirent *eget(struct dirent *parent, char *name)
// 返回一个指定参数的　dirent 指针(即返回在 ecache.entries 中的位置)。
// inum 是 inode 在磁盘 inode 区域的序号, FAT32没有，使用parent 父目录
// 如果给定文件/目录名, 则在ecache缓存中找
// 如果没有给定名字，则在ecache中找空闲的目录项返回
// 2 将 ealloc() 生成的 entry 传入 elock() 函数 加锁
void elock(struct dirent *entry);
// 3 将 ealloc() 生成的 entry 传入 emake()，　对其内容进行一系列操作
struct dirent *edup(struct dirent *entry);
void emake(struct dirent *dp, struct dirent *ep, uint off);
// 4 将　锁释放
void eunlock(struct dirent *entry);

// 最后通常调用eput()
// 表明当前对 entry 对操作结束，e->ref–。
// 如果这时最后一个引用的指针，则需要执行 etrunc(entry) 将 ip->addrs 的内容置零。执行 iupdate(ip) 将前面对 entry 的更新写入磁盘。
void eput(struct dirent *entry);
void etrunc(struct dirent *entry);
void eupdate(struct dirent *entry);

/*
  +--------+
  | ealloc |
  +---+----+
      | eget
      v
  +---+----+
  | elock  |
  +---+----+
      |
      v
+-----+------+
|  estat     |
|  ewrite    |
|  eread     |
|  emake      | 这些是对 entry 内容的操作
|   ...      |
+-----+------+
      |
      v
 +----+----+
 | eunlock |
 +----+----+
      |
      v
 +----+----+
 |  eput   |
 |etrunc   |
 |eupdate  |
 +---------+
*/
// off 是在文件中的字节偏移量。本地变量 tot 纪录着已经读取了多少字节，n - tot 表示还剩多少字节。
// m = min(n - tot, BSIZE - off%BSIZE); 每次将剩余操作字节数与 off 到 block 右边界的大小做对比，取最小的。这样就能保证一次读取的字节数最多不超过 BSIZE。
void estat(struct dirent *ep, struct stat *st);
int eread(struct dirent *entry, int user_dst, uint64_t dst, uint off, uint n);
int ewrite(struct dirent *entry, int user_src, uint64_t src, uint off, uint n);



void eremove(struct dirent *entry);

// 5 Directory Layer
// 目录内部是一种特殊的文件，它对应的 ATTR_DIRECTORY。下面是目录层两个重要的接口:
// 这个函数定位目录 dinode 对应的 datablock，然后从这个 datablock 中依次读取 struct dirent，将其 entry name 与目标 name 比较。如果找到，通过 iget(dp->dev, inum) 获取到这个 entry 对应的 inode。
struct dirent *dirlookup(struct dirent *entry, char *filename, uint *poff);
//static struct dirent *lookup_path(char *path, int parent, char *name)



char *formatname(char *name);

int enext(struct dirent *dp, struct dirent *ep, uint off, int *count);


// 6 Pathname Layer
// ename() 则使用 skipelem() 对路径名进行拆分，返回路径名末尾元素所对应的 dirent:
struct dirent *ename(char *path);
//static char *skipelem(char *path, char *name)
struct dirent *enameparent(char *path, char *name);


// 7 文件描述符








#endif