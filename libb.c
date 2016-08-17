#include <asm/unistd.h>

#define WORDSIZE 8
#define STDOUT 1


static long b_char(long str, long i);
static long b_putchar(long c);

/*
 * In B, the type of a variable declared as `extrn` is not known at
 * compile time; the compiler assumes every variable is an integer.
 * In order for the linker to find externally defined functions,
 * their addresses must be stored in an object type.
 */
extern void *main;
void *char_ = &b_char;
void *putchar = &b_putchar;

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
		"syscall" :
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

static long b_char(long str, long i)
{
	const unsigned char *p;

	p = (unsigned char *)str;

	return p[i];
}

static long b_putchar(long c)
{
	char buf[WORDSIZE];
	const char *p;
	int i, n = 0;

	p = (char *)&c;

	for (i = 0; i < WORDSIZE; i++) {
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
	if (syscall_x86_64(__NR_write, STDOUT, (long)buf, n, 0, 0, 0) != n)
		return (unsigned char) -1;

	return c;
}

void _start()
{
	long status = ((long (*)(void))main)();

	syscall_x86_64(__NR_exit, status, 0, 0, 0, 0, 0);
}
