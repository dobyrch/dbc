#include <asm/unistd.h>
#include "constants.h"

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

static long cstringify(long bstr)
{
	char *cstr, *p;

	cstr = (char *)(bstr << WORDPOW);
	p = cstr;

	while (*p != EOT)
		p++;

	*p = '\0';

	return (long)cstr;
}

static long bstringify(long cstr)
{
	char *p;

	p = (char *)cstr;

	while (*p != '\0')
		p++;

	*p = EOT;

	return cstr >> WORDPOW;
}

__attribute__((aligned(WORDSIZE)))
static long b_chdir(long path)
{
	long r;

	r = syscall_x86_64(__NR_chdir, cstringify(path), 0, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chmod(long path, long mode)
{
	long r;

	r = syscall_x86_64(__NR_chmod, cstringify(path), mode, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chown(long path, long owner)
{
	long r;

	r = syscall_x86_64(__NR_chown, cstringify(path), owner, -1, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_close(long fd)
{
	return syscall_x86_64(__NR_close, fd, 0, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_creat(long path, long mode)
{
	long r;

	r = syscall_x86_64(__NR_creat, cstringify(path), mode, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_exit(long status)
{
	return syscall_x86_64(__NR_exit, status, 0, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
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

/*
 * In B, the type of a variable declared as `extrn` is not known at
 * compile time; the compiler assumes every variable is an integer.
 * In order for the linker to find externally defined functions,
 * their addresses must be stored in an object type.
 */
long (*chdir)() = &b_chdir;
long (*chmod)() = &b_chmod;
long (*chown)() = &b_chown;
long (*close)() = &b_close;
long (*creat)() = &b_creat;
long (*exit_)() = &b_exit;
long (*putchar_)() = &b_putchar;

extern long (*main_)();
long *argv;

void _start()
{
	char cmdline[MAX_STRSIZE];
	long argc;
	char *bstr, *cstr;
	int i;

	asm volatile (
		"mov  8(%%rbp), %0\n\t"
		"lea 16(%%rbp), %1\n\t" :
		"=r" (argc),
		"=r" (argv)
	);

	/*
	 * In B, strings (and all pointers, for that matter) must be
	 * word-aligned.  Let's copy the command line args off of the
	 * stack into a new buffer, fix the alignment, and modify the
	 * terminating char while we're at it (B uses EOT, not NUL).
	 */
	bstr = cmdline;

	for (i = 1; i < argc && i < MAX_ARGS; i++) {
		cstr = (char *)argv[i];

		while ((long)bstr % WORDSIZE)
			bstr++;

		argv[i] = (long)bstr >> WORDPOW;

		while (*cstr != '\0') {
			if (bstr - cmdline >= MAX_STRSIZE)
				goto endfor;

			*bstr++ = *cstr++;
		}

		*bstr++ = EOT;
	}
endfor:

	argv[0] = i - 1;

	exit_(main_());
}
