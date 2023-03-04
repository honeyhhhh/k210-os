#include "include/assert.h"
#include "include/sbi.h"
#include "include/stdio.h"
#include "include/exception.h"
#include "include/console.h"


void __panic(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printk("kernel panic at %s:%d:  ", file, line);
    vprintk(fmt, ap);
    cons_puts("\n");
    va_end(ap);

    irq_disable();
    sbi_shutdown();
    while (1)
    {
        /* code */
    }
    
}

void __warn(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    //printk("kernel warn at %s:%d:  ", file, line);
    vprintk(fmt, ap);
    cons_puts("\n");
    va_end(ap);
}





