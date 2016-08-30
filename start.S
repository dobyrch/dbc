.section .text
.global _start

_start:
#ifdef __amd64__
	mov  (%rsp), %rdi
	lea 8(%rsp), %rsi
#elif __i386__
	mov  (%esp), %eax
	lea 4(%esp), %ebx
	push %ebx
	push %eax
#endif
	call real_start
