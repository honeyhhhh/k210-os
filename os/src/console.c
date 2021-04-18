#include "include/console.h"
#include "include/sbi.h"


void cons_putc(int c)
{
	sbi_console_putchar((unsigned char)c);
}

int cons_getc(void)
{
	return sbi_console_getchar();
}


static void cputch(int c, int *cnt) 
{
	cons_putc(c);
	(*cnt)++;
}

int cons_puts(const char *str) 
{

	int cnt = 0;
	char c;
	while ((c = *str++) != '\0') {

		cputch(c, &cnt);
	}
	return cnt;
}

int cons_puts_n(const char *str, int n) 
{
	int cnt = 0;
	char c;
	while ((c = *str++) != '\0' && cnt < n) {

		cputch(c, &cnt);
	}
	return cnt;
}














