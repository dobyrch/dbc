/*
 * Copyright 2016 Douglas Christman
 *
 * This file is part of dbc.
 *
 * dbc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "platform.h"

static struct {
	char data[STDOUT_BUFSIZE];
	int n;
} stdbuf;

static long asm_syscall(long sc , long a1, long a2, long a3, long a4)
{
	register long ret asm(RT);
	register long rsc asm(SC) = sc;
	register long ra1 asm(A1) = a1;
	register long ra2 asm(A2) = a2;
	register long ra3 asm(A3) = a3;
	register long ra4 asm(A4) = a4;

	asm volatile (
		SYSCALL :
		"=r" (ret) :
		"r"  (rsc),
		"r"  (ra1),
		"r"  (ra2),
		"r"  (ra3),
		"r"  (ra4)
	);

	return ret;
}

static long cstringify(long bstr)
{
	char *cstr, *p;

	cstr = (char *)(bstr << WORDPOW);
	p = cstr;

	/*
	 * If a call to the stdlib passes the same string twice, it may
	 * have already had its terminator modified. Stop when either
	 * EOT or NUL are encountered.
	 */
	while (*p != EOT && *p != '\0')
		p++;

	*p = '\0';

	return (long)cstr;
}

static long bstringify(long cstr)
{
	char *p;

	p = (char *)cstr;

	while (*p != EOT && *p != '\0')
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
	r = asm_syscall(SYS_chdir, path, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chmod(long path, long mode)
{
	long r;

	path = cstringify(path);
	r = asm_syscall(SYS_chmod, path, mode, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_chown(long path, long owner)
{
	long r;

	path = cstringify(path);
	r = asm_syscall(SYS_chown, path, owner, -1, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_close(long fd)
{
	return asm_syscall(SYS_close, fd, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_creat(long path, long mode)
{
	long r;

	path = cstringify(path);
	r = asm_syscall(SYS_open, path, O_CREAT|O_WRONLY|O_TRUNC, mode, 0);
	bstringify(path);

	return r;
}

/*
 * Achtung!
 *
 * This function is guaranteed to yield incorrect results, except (maybe)
 * at midnight on January 1 2370 in Iceland, Portugal, the British Isles,
 * and West Africa.
 */
__attribute__((aligned(WORDSIZE)))
static long b_ctime(long tloc, long date)
{
	char *dp;
	const char *p;
	long t, month, day, hour, min, sec;

	t = *(long *)(tloc << WORDPOW);
	t %= SEC_PER_YEAR;

	month = t / SEC_PER_MONTH;
	t %= SEC_PER_MONTH;

	day = t / SEC_PER_DAY;
	t %= SEC_PER_DAY;

	hour = t / SEC_PER_HOUR;
	t %= SEC_PER_HOUR;

	min = t / SEC_PER_MIN;
	sec = t % SEC_PER_MIN;

	switch (month) {
	case  0: p = "Jan"; break;
	case  1: p = "Feb"; break;
	case  2: p = "Mar"; break;
	case  3: p = "Apr"; break;
	case  4: p = "May"; break;
	case  5: p = "Jun"; break;
	case  6: p = "Jul"; break;
	case  7: p = "Aug"; break;
	case  8: p = "Sep"; break;
	case  9: p = "Oct"; break;
	case 10: p = "Nov"; break;
	case 11: p = "Dec"; break;
	default: p = "???"; break;
	}

	dp = (char *)(date << WORDPOW);

	while (*p != '\0')
		*dp++ = *p++;

	*dp++ = ' ';
	*dp++ = '0' + day/10;
	*dp++ = '0' + day%10;
	*dp++ = ' ';
	*dp++ = '0' + hour/10;
	*dp++ = '0' + hour%10;
	*dp++ = ':';
	*dp++ = '0' + min/10;
	*dp++ = '0' + min%10;
	*dp++ = ':';
	*dp++ = '0' + sec/10;
	*dp++ = '0' + sec%10;
	*dp++ = EOT;

	return 0;
}

__attribute__((aligned(WORDSIZE)))
static long b_execl(long path, ...)
{
	va_list ap;
	long av[MAX_ARGS];
	long envp[1];
	long arg;
	int count, i;

	va_start(ap, path);

	for (i = 1; (arg = va_arg(ap, long)); i++) {
		if (i > MAX_ARGS - 2)
			return -1;

		av[i] = arg;
	}

	count = i - 1;
	av[0] = path;

	for (i = 0; i <= count; i++)
		av[i] = cstringify(av[i]);

	av[i] = 0;
	envp[0] = 0;

	asm_syscall(SYS_execve, av[0], (long)av, (long)envp, 0);

	/* If we're still here, execve must have failed */
	for (i = 0; i < count; i++)
		bstringify(av[i]);

	return -1;
}

__attribute__((aligned(WORDSIZE)))
static long b_execv(long path, long args, long count)
{
	long av[MAX_ARGS];
	long envp[1];
	long *p;
	int i;

	/* We need space for the executable name and a NULL terminator */
	if (count > MAX_ARGS - 2)
		return -1;

	p = (long *)(args << WORDPOW);

	for (i = 1; i <= count; i++)
		av[i] = cstringify(p[i-1]);

	av[i] = 0;
	av[0] = cstringify(path);
	envp[0] = 0;

	asm_syscall(SYS_execve, av[0], (long)av, (long)envp, 0);

	for (i = 0; i <= count; i++)
		bstringify(av[i]);

	return -1;
}

__attribute__((aligned(WORDSIZE)))
static long b_exit(long status)
{
	asm_syscall(SYS_write, STDOUT_FILENO, (long)stdbuf.data, stdbuf.n, 0);
	return asm_syscall(SYS_exit, status, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_fork()
{
	return asm_syscall(SYS_fork, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_fstat(long fd, long status)
{
	struct stat buf;
	long *s;
	long r;

	r = asm_syscall(SYS_fstat, fd, (long)&buf, 0, 0);

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
static long b_getchar()
{
	long c;

	if (asm_syscall(SYS_read, STDIN_FILENO, (long)&c, 1, 0) != 1)
		return -1;

	return c;
}

__attribute__((aligned(WORDSIZE)))
static long b_getuid()
{
	return asm_syscall(SYS_getuid, 0, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_gtty(long fd, long ttystat)
{
	struct termios buf;
	long *t;
	long r;

	r = asm_syscall(SYS_ioctl, fd, TGET, (long)&buf, 0);

	t = (long *)(ttystat << WORDPOW);
	t[0] = buf.c_iflag;
	t[1] = buf.c_oflag;
	t[2] = buf.c_cflag;
	t[3] = buf.c_lflag;

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_lchar(long s, long i, long c)
{
	char *str;

	str = (char *)(s << WORDPOW);
	str[i] = c;

	return c;
}

__attribute__((aligned(WORDSIZE)))
static long b_link(long oldpath, long newpath)
{
	long r;

	oldpath = cstringify(oldpath);
	newpath = cstringify(newpath);
	r = asm_syscall(SYS_link, oldpath, newpath, 0, 0);
	bstringify(oldpath);
	bstringify(newpath);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_mkdir(long path, long mode)
{
	long r;

	path = cstringify(path);
	return asm_syscall(SYS_mkdir, mode, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_open(long path, long flag)
{
	long r;

	if (flag < 0 || flag > 2)
		flag = O_RDWR;

	path = cstringify(path);
	r = asm_syscall(SYS_open, path, flag, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_putchar(long c)
{
	const char *p;
	int i;

	p = (char *)&c;

	for (i = 0; i < WORDSIZE; i++) {
		if (*p == '\0')
			continue;

		stdbuf.data[stdbuf.n] = *p;
		stdbuf.n++;

		if (*p == '\n' || stdbuf.n >= STDOUT_BUFSIZE) {
			asm_syscall(SYS_write, STDOUT_FILENO,
				(long)stdbuf.data, stdbuf.n, 0);
			stdbuf.n = 0;
		}

		p++;
	}

	return 0;
}

__attribute__((aligned(WORDSIZE)))
static long b_printn(long n, long b)
{
	long a;

	if ((a = n/b))
		b_printn(a, b);

	return b_putchar(n%b + '0');
}

__attribute__((aligned(WORDSIZE)))
static long b_printf(long fmt, ...)
{
	va_list ap;
	long x, c, i, j;

	i = 0;
	va_start(ap, fmt);
loop:
	while ((c = b_char(fmt, i++)) != '%') {
		if (c == EOT) {
			va_end(ap);
			return 0;
		}

		b_putchar(c);
	}


	switch ((c = b_char(fmt, i++))) {
	case 'd':
	case 'o':
		x = va_arg(ap, long);
		if (x < 0) {
			x = -x;
			b_putchar('-');
		}

		b_printn(x, c == 'o' ? 8 : 10);
		goto loop;

	case 'c':
		x = va_arg(ap, long);
		b_putchar(x);
		goto loop;

	case 's':
		x = va_arg(ap, long);
		j = 0;
		while ((c = b_char(x, j++)) != EOT)
			b_putchar(c);
		goto loop;
	}

	b_putchar('%');
	i--;
	goto loop;
}

__attribute__((aligned(WORDSIZE)))
static long b_read(long fd, long buf, long count)
{
	buf <<= WORDPOW;
	return asm_syscall(SYS_read, fd, buf, count, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_seek(long fd, long offset, long whence)
{
	return asm_syscall(SYS_lseek, fd, offset, whence, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_setuid(long id)
{
	return asm_syscall(SYS_setuid, id, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_stat(long path, long status)
{
	struct stat buf;
	long *s;
	long r;

	path = cstringify(path);
	r = asm_syscall(SYS_stat, path, (long)&buf, 0, 0);
	bstringify(path);

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
static long b_stty(long fd, long ttystat)
{
	struct termios buf;
	long *t;

	t = (long *)(ttystat << WORDPOW);
	buf.c_iflag = t[0];
	buf.c_oflag = t[1];
	buf.c_cflag = t[2];
	buf.c_lflag = t[3];

	return asm_syscall(SYS_ioctl, fd, TSET, (long)&buf, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_time(long tloc)
{
	struct timespec tspec;
	long *t;
	long r;

	r = asm_syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&tspec, 0, 0);

	t = (long *)(tloc << WORDPOW);
	t[0] = tspec.tv_sec;

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_unlink(long path)
{
	long r;

	path = cstringify(path);
	r = asm_syscall(SYS_unlink, path, 0, 0, 0);
	bstringify(path);

	return r;
}

__attribute__((aligned(WORDSIZE)))
static long b_wait()
{
	return asm_syscall(SYS_wait4, -1, 0, 0, 0);
}

__attribute__((aligned(WORDSIZE)))
static long b_write(long fd, long buf, long count)
{
	buf <<= WORDPOW;
	return asm_syscall(SYS_write, fd, buf, count, 0);
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
long (*ctime_)() = &b_ctime;
long (*execl_)(long, ...) = &b_execl;
long (*execv_)() = &b_execv;
long (*exit_)() = &b_exit;
long (*fork_)() = &b_fork;
long (*fstat_)() = &b_fstat;
long (*getchar_)() = &b_getchar;
long (*getuid_)() = &b_getuid;
long (*gtty_)() = &b_gtty;
long (*lchar_)() = &b_lchar;
long (*link_)() = &b_link;
long (*mkdir_)() = &b_mkdir;
long (*open_)() = &b_open;
long (*printf_)(long, ...) = &b_printf;
long (*printn_)() = &b_printn;
long (*putchar_)() = &b_putchar;
long (*read_)() = &b_read;
long (*seek_)() = &b_seek;
long (*setuid_)() = &b_setuid;
long (*stat_)() = &b_stat;
long (*stty_)() = &b_stty;
long (*time_)() = &b_time;
long (*unlink_)() = &b_unlink;
long (*wait_)() = &b_wait;
long (*write_)() = &b_write;

extern long (*main_)();
long *argv;

void real_start(long argc, long argvp)
{
	char cmdline[MAX_STRSIZE];
	char *bstr, *cstr;
	int i;

	argv = (long *)argvp;

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
