#include "include/stdio.h"
#include "include/stddef.h"
#include "include/console.h"
#include "include/string.h"
#include "include/spinlock.h"


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


static struct
{
    struct spinlock lock;
    int locking;
} pr;




int getchar()
{
    return cons_getc();
}

void putchar(int c)
{
    cons_putc(c);
}



int puts(const char *s)
{
    return cons_puts(s);
}

static char digits[] = "0123456789abcdef";

// base 进制 10,16      sign 1 符号处理
static void printint(int xx, int base, int sign)
{
    char buf[16 + 1];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    buf[16] = 0;
    i = 15;
    do
    {
        buf[i--] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i--] = '-';
    i++;
    if (i < 0)
        puts("printint error");
    cons_puts(buf+i);
    //out(stdout, buf + i, 16 - i); //static void out(int f, const char *s, size_t l){  write(f, s, l); }
}

//ssize_t read(int, void *, size_t);
//ssize_t write(int, const void *, size_t);

static void printptr(uint64_t x)
{
    int i = 0, j;
    char buf[32 + 1];
    buf[i++] = '0';
    buf[i++] = 'x';
    for (j = 0; j < (sizeof(uint64_t) * 2); j++, x <<= 4)
        buf[i++] = digits[x >> (sizeof(uint64_t) * 8 - 4)];
    buf[i] = 0;
    //out(stdout, buf, i);
    cons_puts(buf);
}

void printk(const char *fmt, ...)
{
    va_list ap;


    //acquire(&pr.lock); 
    va_start(ap, fmt);
    
    vprintk(fmt, ap);

    va_end(ap);

    //release(&pr.lock); 
}


// Print to the console. only understands %d, %x, %p, %s.
void vprintk(const char *fmt, va_list ap)
{
    //va_list ap;
    int l = 0;
    char *a, *z, *s = (char *)fmt;
    //int f = stdout;


    //va_start(ap, fmt);
    for (;;)
    {
        if (!*s)
            break;
        for (a = s; *s && *s != '%'; s++)
            ;
        for (z = s; s[0] == '%' && s[1] == '%'; z++, s += 2)
            ;
        l = z - a;
        //out(f, a, l);
        cons_puts_n(a, l);  //%s
        if (l)
            continue;
        if (s[1] == 0)
            break;
        switch (s[1])
        {
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, uint64_t));
            break;
        case 's':
            if ((a = va_arg(ap, char *)) == 0)
                a = "(null)";
            l = strnlen(a, 200);
            //out(f, a, l);
            cons_puts(a);
            break;
        default:
            // Print unknown % sequence to draw attention.
            putchar('%');
            putchar(s[1]);
            break;
        }
        s += 2;
    }
    //va_end(ap);

}


void printinit()
{
    initlock(&pr.lock, "pr");
    pr.locking = 1;


    //cons_puts("print init\n");
}
