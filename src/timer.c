#include "include/timer.h"
#include "include/riscv_asm.h"
#include "include/sbi.h"
#include "include/stdio.h"
#include "include/spinlock.h"
#include "include/assert.h"
#include "include/console.h"
#include "include/sysctl.h"
#include "include/sleep.h"
#include "include/clint.h"
#include "include/rtc.h"
#include "include/sleep.h"
#include "include/proc.h"

// CLOCK_FREQ
const uint64_t QEMU_CLOCK_FREQ = 10000000; //10000000;
const uint64_t K210_CLOCK_FREQ = 403000000 / 62;
// 1000ms = 1s
const uint64_t MSEC_PER_SEC = 1000;
// 时钟中断间隔
static uint64_t INTERVAL = (390000000 / 200);//100000; // (390000000 / 200)
// 触发时钟中断计数 < cycle
struct spinlock tickslock;
uint64_t TICKS = 0;      
// 每秒触发时钟中断次数?   对于QEMU，时钟频率10MHz，每过1s，mtime增加10 000 000，也就是100ns，如果设置时间间隔100 000，每秒将发生100次中断
const uint64_t TICKS_PER_SEC = 100;

inline uint64_t get_time()
{
    return csr_read(CSR_TIME);
}

uint64_t get_time_ms()
{
    return get_time() / (K210_CLOCK_FREQ / MSEC_PER_SEC);
}

// supervisor-mode cycle counter
static inline uint64_t r_time()
{
    uint64_t x;
    // asm volatile("csrr %0, time" : "=r" (x) );
    // this instruction will trap in SBI
    asm volatile("rdtime %0" : "=r" (x) );
    return x;
}


inline uint64_t get_time_v2()
{
#if __riscv_xlen == 64
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
#else
    uint32_t lo, hi, tmp;
    __asm__ __volatile__(
        "1:\n"
        "rdtimeh %0\n"
        "rdtime %1\n"
        "rdtimeh %2\n"
        "bne %0, %2, 1b"
        : "=&r"(hi), "=&r"(lo), "=&r"(tmp)
    );
    return ((uint64_t)hi << 32) | lo;
#endif
}

uint64_t get_cycle()
{
    asm volatile("fence.i");
    return csr_read(CSR_CYCLE);
}


// SIE.STIE = 1 并 set_timer
void timer_init()
{
    initlock(&tickslock, "time");
    csr_set(CSR_SIE, IE_TIE);  //in trapinit
    //set_next_trigger();
    // printk("timer start work....\n");
    // printk("%d\n", r_time());
    // printk("%d\n", get_time());
    // printk("%d\n", get_cycle()); //获取CPU开机至今的时钟数。可以用使用这个函数精准的确定程序运行时钟。可以配合sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)计算运行的时间。
    
    /*
    printk("cpu clk=%d\n", sysctl_clock_get_freq(SYSCTL_CLOCK_CPU));


    printk("%d\n",clint_timer_get_freq());



    TimeVal tp;
    uint64_t clint_usec = clint->mtime / (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / CLINT_CLOCK_DIV / 1000000UL);

    printk("%d\n", clint_usec);

    tp.sec = clint_usec / 1000000UL;
    tp.usec = clint_usec % 1000000UL;

    printk("sec :[%d]\n", tp.sec);
    printk("usec :[%d]\n", tp.usec);


    rtc_timer_set(2021, 5, 20, 13, 14, 0);
    struct tm *tm = rtc_timer_get_tm();
    printk("sec :[%d]\n", tm->tm_sec);

    ksleep(1);

    tm = rtc_timer_get_tm();

    printk("sec :[%d]\n", tm->tm_sec);

    printk("min :[%d]\n", tm->tm_min);
    printk("hour:[%d]\n", tm->tm_hour);
    printk("mday:[%d]\n", tm->tm_mday);
    printk("mon: [%d]\n", tm->tm_mon);
    printk("year:[%d]\n", tm->tm_year);
    printk("wday:[%d]\n", tm->tm_wday);
    printk("yday:[%d]\n", tm->tm_yday);
    printk("[%d]\n", tm->tm_isdst);
    */

    rtc_init();
    sys_time_init(2021, 5, 28, 22, 37, 0); // 1622212621
}

//　设置当前时间
int sys_time_init(int year, int month, int day, int hour, int minute, int second)
{
    return rtc_timer_set(year, month, day, hour, minute, second);
}

// 获取当前时间
struct tm *sys_time_get()
{
    return rtc_timer_get_tm();
}

// 获取时间戳
TimeVal tm2tamp(struct tm *tm, int utc_diff)
{
    TimeVal time;

    const int mon_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    long tyears,tdays,leap_years,utc_hrs;
    int is_leap;
    int i,ryear;

    //判断闰年
    ryear = tm->tm_year + 1900;
    is_leap = ((ryear%100!=0 && ryear%4==0) || (ryear%400==0) ) ? 1 : 0;

    tyears = tm->tm_year - 70;  //时间戳从1970年开始算起
    if(tm->tm_mon < 1 && is_leap ==1 )
    {
        leap_years = (tyears + 2) / 4 - 1;  //1970年不是闰年，从1972年开始闰年  //闰年的月份小于1，需要减去一天
    }
    else
    {
        leap_years = (tyears + 2) / 4 ;
    }
    tdays = 0;
    for (i=0; i < tm->tm_mon; ++i)
    {
        tdays += mon_days[i];
    }
    tdays += tm->tm_mday - 1;  //减去今天
    tdays += tyears * 365 + leap_years;
    utc_hrs = tm->tm_hour - utc_diff;   // 时区

    time.sec =  (tdays * 86400) + (utc_hrs * 3600) + (tm->tm_min * 60) + tm->tm_sec;
    time.usec = 0;

    return time;
}


void set_next_trigger()
{
    set_timer(r_time() + INTERVAL);
}

void timer_handle()
{
    acquire(&tickslock);
    TICKS++;
    release(&tickslock);

    struct TaskControlBlock *p = mytask();
    if (p == NULL) {
        set_next_trigger();
        return;
    }
    if ((csr_read(CSR_SSTATUS) & SSTATUS_SPP) == 0) 
    {  // in user
        acquire(&p->tcb_lock);
        p->utimes++;
        release(&p->tcb_lock);
        
    } else {  //in kernel
        acquire(&p->tcb_lock);
        p->stimes++;
        release(&p->tcb_lock);
    }




    set_next_trigger();


    // 分时任务此处调用 suspend 
}