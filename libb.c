#include <asm/unistd.h>
#include "constants.h"

static long b_char(long str, long i);
static long b_putchar(long c);

/*
 * In B, the type of a variable declared as `extrn` is not known at
 * compile time; the compiler assumes every variable is an integer.
 * In order for the linker to find externally defined functions,
 * their addresses must be stored in an object type.
 */
long (*char_)() = &b_char;
long (*putchar)() = &b_putchar;

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

/* TODO: Use modified B implementation */
static long b_char(long str, long i)
{
	const unsigned char *p;

	p = (unsigned char *)(str << WORDPOW);

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

	if (syscall_x86_64(__NR_write, STDOUT, (long)buf, n, 0, 0, 0) != n)
		return -1;

	return c;
}

extern long (*main)();
long *argv;

void _start()
{
	long argc, status;
	char *p;
	int i;

	asm volatile (
		"mov  8(%%rbp), %0\n\t"
		"lea 16(%%rbp), %1\n\t" :
		"=r" (argc),
		"=r" (argv)
	);

	argv[0] = argc - 1;

	for (i = 1; i <= argv[0]; i++) {
		p = (char *)argv[i];

		while (*p != '\0')
			p++;

		*p = EOT;
	}

	status = main();
	syscall_x86_64(__NR_exit, status, 0, 0, 0, 0, 0);
}
