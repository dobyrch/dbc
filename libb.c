#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
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
static long b_char(long s, long i)
{
	const unsigned char *str;

	/* make unsigned so chars > 127 aren't sign extended */
	str = (unsigned char *)(s << WORDPOW);

	return str[i];
}

__attribute__((aligned(WORDSIZE)))
static long b_chdir(long path)
{
	long r;

	path = cstringify(path);
	r = syscall_x86_64(__NR_chdir, path, 0, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chmod(long path, long mode)
{
	long r;

	path = cstringify(path);
	r = syscall_x86_64(__NR_chmod, path, mode, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chown(long path, long owner)
{
	long r;

	path = cstringify(path);
	r = syscall_x86_64(__NR_chown, path, owner, -1, 0, 0, 0);
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

	path = cstringify(path);
	r = syscall_x86_64(__NR_creat, path, mode, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_exit(long status)
{
	return syscall_x86_64(__NR_exit, status, 0, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_fork()
{
	return syscall_x86_64(__NR_fork, 0, 0, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_fstat(long fd, long status)
{
	struct stat buf;
	long *s;
	long r;

	r = syscall_x86_64(__NR_fstat, fd, (long)&buf, 0, 0, 0, 0);

	s = (long *)(status << WORDPOW);
	s[0]  = buf.st_dev;
	s[1]  = buf.st_ino;
	s[2]  = buf.st_mode;
	s[3]  = buf.st_nlink;
	s[4]  = buf.st_uid;
	s[5]  = buf.st_gid;
	s[6]  = buf.st_rdev;
	s[7]  = buf.st_size;
	s[8]  = buf.st_blksize;
	s[9]  = buf.st_blocks;
	s[10] = buf.st_atim.tv_sec;
	s[11] = buf.st_atim.tv_nsec;
	s[12] = buf.st_mtim.tv_sec;
	s[13] = buf.st_mtim.tv_nsec;
	s[14] = buf.st_ctim.tv_sec;
	s[15] = buf.st_ctim.tv_nsec;

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_getuid()
{
	return syscall_x86_64(__NR_getuid, 0, 0, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_link(long oldpath, long newpath)
{
	long r;

	oldpath = cstringify(oldpath);
	newpath = cstringify(newpath);
	r = syscall_x86_64(__NR_link, oldpath, newpath, 0, 0, 0, 0);
	bstringify(oldpath);
	bstringify(newpath);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_mkdir(long path, long mode)
{
	long r;

	path = cstringify(path);
	return syscall_x86_64(__NR_mkdir, mode, 0, 0, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_open(long path, long mode)
{
	long r;

	if (mode < 0 || mode > 2)
		mode = O_RDWR;

	path = cstringify(path);
	r = syscall_x86_64(__NR_open, path, 0, mode, 0, 0, 0);
	bstringify(path);

	return r;
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

	if (syscall_x86_64(__NR_write, STDOUT_FILENO, (long)buf, n, 0, 0, 0) != n)
		return -1;

	return c;
}

__attribute__((aligned(WORDSIZE)))
static long b_read(long fd, long buf, long count)
{
	long r;

	buf <<= WORDPOW;
	r = syscall_x86_64(__NR_read, fd, buf, count, 0, 0, 0);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_seek(long fd, long offset, long whence)
{
	return syscall_x86_64(__NR_lseek, fd, offset, whence, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_setuid(long id)
{
	return syscall_x86_64(__NR_setuid, id, 0, 0, 0, 0, 0);
}

/*
 * In B, the type of a variable declared as `extrn` is not known at
 * compile time; the compiler assumes every variable is an integer.
 * In order for the linker to find externally defined functions,
 * their addresses must be stored in an object type.
 *
 * All symbols are suffixed with "_" to avoid clashes with C stdlib
 * functions. They are renamed after compilation with objcopy.
 */
long (*char_)() = &b_char;
long (*chdir_)() = &b_chdir;
long (*chmod_)() = &b_chmod;
long (*chown_)() = &b_chown;
long (*close_)() = &b_close;
long (*creat_)() = &b_creat;
long (*exit_)() = &b_exit;
long (*fork_)() = &b_fork;
long (*fstat_)() = &b_fstat;
long (*getuid_)() = &b_getuid;
long (*link_)() = &b_link;
long (*mkdir_)() = &b_mkdir;
long (*open_)() = &b_open;
long (*putchar_)() = &b_putchar;
long (*read_)() = &b_read;
long (*seek_)() = &b_seek;
long (*setuid_)() = &b_setuid;

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
