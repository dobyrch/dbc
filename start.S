.section .text
.global _start

/*
 * Command line arguments are passed on the stack.
 * Pass argc and argv to real_start according to the
 * calling conventions of the target platform.
 */

_start:
#ifdef __amd64__
	mov  (%rsp), %rdi
	lea 8(%rsp), %rsi
	jmp real_start
#elif __i386__
	mov  (%esp), %eax
	lea 4(%esp), %ebx
	push %ebx
	push %eax
	call real_start
#elif __arm__
	ldr r0, [sp]
	add r1, sp, #4
	b real_start
#endif
