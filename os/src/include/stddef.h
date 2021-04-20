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


/* point and address */
typedef uintptr_t size_t;
typedef intptr_t ssize_t;

typedef int pid_t;
#define NULL ((void *)0)


/* 可变参数 */
typedef __builtin_va_list va_list;
#define va_start(ap, last) (__builtin_va_start(ap, last))
#define va_arg(ap, type) (__builtin_va_arg(ap, type))
#define va_end(ap) (__builtin_va_end(ap))
#define va_copy(d, s) (__builtin_va_copy(d, s))



struct list_head {
	struct list_head *next, *prev;
};


typedef struct
{
    uint64_t sec;  // 自 Unix 纪元起的秒数
    uint64_t usec; // 微秒数
} TimeVal;








#endif
