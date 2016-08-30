#ifdef __amd64__
	#define TYPE_INT LLVMInt64Type()
	#define WORDPOW 3
	#define SYSCALL "syscall"
	#define SC "rax"
	#define A1 "rdi"
	#define A2 "rsi"
	#define A3 "rdx"
	#define A4 "r10"
#elif __i386__
	#define TYPE_INT LLVMInt32Type()
	#define WORDPOW 2
	#define SYSCALL "int $0x80"
	#define SC "eax"
	#define A1 "ebx"
	#define A2 "ecx"
	#define A3 "edx"
	#define A4 "esi"
#else
	#error unknown architecture
#endif
