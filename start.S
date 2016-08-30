.section .text
.global _start

_start:
#ifdef __amd64__
	mov  (%rsp), %rdi
	lea 8(%rsp), %rsi
#elif __i386__
	; args are already on the stack,
	; right where they need to be
#endif
	jmp real_start
