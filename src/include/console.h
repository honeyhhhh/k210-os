#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "stddef.h"
void consoleinit();
int consoleread(int user_dst, uint64_t dst, int n);
int consolewrite(int user_src, uint64_t src, int n);


void cons_putc(int ch);
int cons_getc(void);

int cons_puts(const char *str);
int cons_puts_n(const char *str, int n);


#endif
