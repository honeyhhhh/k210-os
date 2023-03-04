#ifndef _STDDEF_H
#define _STDDEF_H



typedef int bool;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int  uint32_t;
typedef unsigned int uint;
typedef unsigned long long uint64_t;

typedef uint64_t uintptr_t;
typedef int64_t intptr_t;


#define ULONG_MAX (0xffffffffffffffffULL)
#define LONG_MAX (0x7fffffffffffffffLL)
#define INTMAX_MAX LONG_MAX
#define UINT_MAX (0xffffffffU)
#define INT_MAX (0x7fffffff)
#define UCHAR_MAX (0xffU)
#define CHAR_MAX (0x7f)

#define NPROC        16  // maximum number of processes
#define NCPU          2  // maximum number of CPUs

/* point and address */
typedef uintptr_t size_t;
typedef intptr_t ssize_t;


#define NULL ((void *)0)


/* 可变参数 */
typedef __builtin_va_list va_list;
#define va_start(ap, last) (__builtin_va_start(ap, last))
#define va_arg(ap, type) (__builtin_va_arg(ap, type))
#define va_end(ap) (__builtin_va_end(ap))
#define va_copy(d, s) (__builtin_va_copy(d, s))



typedef struct
{
    uint64_t sec;  // 自 Unix 纪元起的秒数
    uint64_t usec; // 微秒数
} TimeVal;

struct tms              
{                     
	long tms_utime; // 用户态下 
	long tms_stime;  // 内核态下
	long tms_cutime;  // 子进程用户态下
	long tms_cstime; // 子进程内核态下
};


struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;   // -1900
    int tm_wday;  // Alarm Week. Range [0,6]. 0 is Sunday. 星期x
    int tm_yday;   // 从每年的1月1日开始的天数 要判断是否是闰年
    int tm_isdst;  //-1
};




typedef unsigned int mode_t;
typedef long int off_t;

struct kstat {
        uint64_t st_dev;
        uint64_t st_ino;
        mode_t st_mode;
        uint32_t st_nlink;
        uint32_t st_uid;
        uint32_t st_gid;
        uint64_t st_rdev;
        unsigned long __pad;
        off_t st_size;
        uint32_t st_blksize;
        int __pad2;
        uint64_t st_blocks;
        long st_atime_sec;
        long st_atime_nsec;
        long st_mtime_sec;
        long st_mtime_nsec;
        long st_ctime_sec;
        long st_ctime_nsec;
        unsigned __unused[2];
};


struct linux_dirent64 {
        uint64_t        d_ino;
        int64_t         d_off;
        unsigned short  d_reclen;
        unsigned char   d_type;
        char            d_name[];
};






#define false 0
#define true  1


#define readb(addr) (*(volatile uint8_t *)(addr))
#define readw(addr) (*(volatile uint16_t *)(addr))
#define readd(addr) (*(volatile uint32_t *)(addr))
#define readq(addr) (*(volatile uint64_t *)(addr))

#define writeb(v, addr)                      \
    {                                        \
        (*(volatile uint8_t *)(addr)) = (v); \
    }
#define writew(v, addr)                       \
    {                                         \
        (*(volatile uint16_t *)(addr)) = (v); \
    }
#define writed(v, addr)                       \
    {                                         \
        (*(volatile uint32_t *)(addr)) = (v); \
    }
#define writeq(v, addr)                       \
    {                                         \
        (*(volatile uint64_t *)(addr)) = (v); \
    }


#define NELEM(x) (sizeof(x)/sizeof((x)[0]))


#endif
