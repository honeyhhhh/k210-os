#include "include/assert.h"
#include "include/sbi.h"
#include "include/stdio.h"
#include "include/exception.h"


void __panic(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("kernel panic at %s:%d:  ", file, line);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);

    irq_disable();
    sbi_shutdown();
    while (1)
    {
        /* code */
    }
    
}






