/*
 * Running with
 *
 *	clang -c example.c $(llvm-config --cflags)
 *	clang++ example.o -o example $(llvm-config --ldflags --libs --system-libs)
 *	./example
 *
 * outputs
 *
 *	; ModuleID = 'example'
 *
 *	@global_int = global i64 42
 *	@global_array = global [1 x i64] [[1 x i64] ptrtoint (i64* @global_int to i64)]
 *	Global variable initializer type does not match global variable type!
 *	[1 x i64]* @global_array*
 *
 * Note that the return type of ptrtoint is [1 x i64], not i64.  If the
 * output is manually edited to change the type of ptrtoint to i64, llc
 * compiles it without error.
 */

#include <stddef.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

#define TYPE_INT LLVMInt64Type()
#define TYPE_ARRAY LLVMArrayType(TYPE_INT, 1)

int main(void)
{
	LLVMModuleRef module;
	LLVMValueRef global_int, global_array;
	LLVMValueRef int_init, array_init;
	LLVMValueRef int_ptr;

	module = LLVMModuleCreateWithName("example");

	global_int = LLVMAddGlobal(module, TYPE_INT, "global_int");
	global_array = LLVMAddGlobal(module, TYPE_ARRAY, "global_array");

	int_init = LLVMConstInt(TYPE_INT, 42, 0);
	LLVMSetInitializer(global_int, int_init);

	int_ptr = LLVMConstPtrToInt(global_int, TYPE_INT);
	array_init = LLVMConstArray(TYPE_ARRAY, &int_ptr, 1);
	LLVMSetInitializer(global_array, array_init);

	LLVMDumpModule(module);
	LLVMVerifyModule(module, LLVMPrintMessageAction, NULL);
}
