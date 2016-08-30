#ifdef __amd64__
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
