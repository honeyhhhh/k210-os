#ifndef _CONSOLE_H
#define _CONSOLE_H

void cons_putc(int ch);
int cons_getc(void);

int cons_puts(const char *str);
int cons_puts_n(const char *str, int n);


#endif
