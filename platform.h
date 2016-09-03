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

#include <sys/ioctl.h>

/*
 * All architectures require the following definitions:
 *
 *     TYPE_INT:  LLVMInt[NN]Type(), where NN is the word size in bits
 *     WORDPOW:   log2(NN / 8)
 *     SYSCALL:   The assembly instruction used to invoke a system call
 *     RT:        The register that holds the return value of a syscall
 *     SC:        The register that holds the system call number
 *     A1 - A4:   The registers that hold arguments 1 - 4 of a syscall
 *
 * See `man syscall` for calling conventions on other architectures.
 * New additions also require an entry in start.S to set up command-
 * line arguments.
 */
#if __amd64__
	#define TYPE_INT LLVMInt64Type()
	#define WORDPOW 3
	#define SYSCALL "syscall"
	#define RT "rax"
	#define SC "rax"
	#define A1 "rdi"
	#define A2 "rsi"
	#define A3 "rdx"
	#define A4 "r10"
#elif __i386__
	#define TYPE_INT LLVMInt32Type()
	#define WORDPOW 2
	#define SYSCALL "int $0x80"
	#define RT "eax"
	#define SC "eax"
	#define A1 "ebx"
	#define A2 "ecx"
	#define A3 "edx"
	#define A4 "esi"
#elif __arm__
	#define TYPE_INT LLVMInt32Type()
	#define WORDPOW 2
	#define SYSCALL "swi 0x0"
	#define RT "r0"
	#define SC "r7"
	#define A1 "r0"
	#define A2 "r1"
	#define A3 "r2"
	#define A4 "r3"
#else
	#error unknown architecture
#endif

/* Linux and the BSDs use different names for tty ioctls */
#if defined(TCGETS)
	#define TGET TCGETS
	#define TSET TCSETS
#elif defined(TIOCGETA)
	#define TGET TIOCGETA
	#define TSET TIOCSETA
#else
	#error Unknown platform
#endif
