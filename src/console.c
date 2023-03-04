#include "include/console.h"
#include "include/sbi.h"
#include "include/file.h"
#include "include/spinlock.h"
#include "include/sem.h"
#include "include/proc.h"
#include "include/list.h"




#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x










void cons_putc(int c) 
{
	if(c == BACKSPACE){
		// if the user typed backspace, overwrite with a space.
		sbi_console_putchar('\b');
		sbi_console_putchar(' ');
		sbi_console_putchar('\b');
	} else {
		sbi_console_putchar(c);
	}
}

struct {
	struct spinlock lock;
	
	// input
	#define INPUT_BUF 128
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index

	struct list waitr;
} cons;

int consolewrite(int user_src, uint64_t src, int n)
{
	int i;

	//acquire(&cons.lock);
	for(i = 0; i < n; i++){
		char c;
		if(either_copyin(&c, user_src, src+i, 1) == -1)
			break;
		sbi_console_putchar(c);
	}
	//release(&cons.lock);

	return i;
}

int consoleread(int user_dst, uint64_t dst, int n)
{
	uint target;
	int c;
	char cbuf;

	target = n;
	//acquire(&cons.lock);
	while(n > 0){
		// wait until interrupt handler has put some
		// input into cons.buffer.
		while(cons.r == cons.w)
		{
			if(mytask()->task_status == Zombie){ //killed
				release(&cons.lock);
				return -1;
			}
			sleep(&cons.waitr, &cons.lock);
		}

		c = cons.buf[cons.r++ % INPUT_BUF];

		if(c == C('D'))
		{  // end-of-file
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				cons.r--;
			}
			break;
		}

		// copy the input byte to the user-space buffer.
		cbuf = c;
		if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
			break;

		dst++;
		--n;

		if(c == '\n'){
		// a whole line has arrived, return to
		// the user-level read().
			break;
		}
	}
	//release(&cons.lock);

	return target - n;
}

void consoleinit(void)
{
	initlock(&cons.lock, "cons");
	list_init(&cons.waitr);
	cons.e = cons.w = cons.r = 0;
	
	// connect read and write system calls
	// to consoleread and consolewrite.
	devsw[CONSOLE].read = consoleread;
	devsw[CONSOLE].write = consolewrite;
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














