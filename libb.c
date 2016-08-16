#include <asm/unistd.h>

static long syscall_x86_64(long sc , long a1, long a2, long a3, long a4, long a5, long a6)
{
	register long rax asm("rax") = sc;
	register long rdi asm("rdi") = a1;
	register long rsi asm("rsi") = a2;
	register long rdx asm("rdx") = a3;
	register long r10 asm("r10") = a4;
	register long r8  asm("r8")  = a5;
	register long r9  asm("r9")  = a6;

	asm volatile (
		"syscall\n\t" :
		"+r" (rax) :
		"r" (rdi),
		"r" (rsi),
		"r" (rdx),
		"r" (r10),
		"r" (r8),
		"r" (r9)
	);

	return rax;
}

static long b_putchar(long c)
{
	/* TODO: put MAX_CHARSIZE (renamed to WORDSIZE) in shared header */
	char buf[8];
	const char *p;
	int i, n = 0;

	p = (char *)&c;

	/* TODO: ...and remove this magic number */
	for (i = 0; i < 8; i++) {
		if (*p == '\0')
			continue;

		buf[n] = *p;
		n++;
		p++;
	}

	/*
	 * TODO: Add header that defines EOF, STDOUT, and other constants
	 * (since stdlib headers can't be included here); EOF should
	 * probably be defined as unsigned char to save from typing out
	 * the cast here and in codegen.c
	 */
	if (syscall_x86_64(__NR_write, 1, (long)buf, n, 0, 0, 0) != n)
		return (unsigned char) -1;

	return c;
}


void *putchar = &b_putchar;

extern int main();

void _start()
{
	int status = main();

	syscall_x86_64(__NR_exit, status, 0, 0, 0, 0, 0);
}
