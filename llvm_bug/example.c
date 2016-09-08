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
 *	@global_int = global i64 31
 *	@global_array = global [2 x i64] [[2 x i64] 42, [2 x i64] ptrtoint (i64* @global_int to i64)]
 *	Global variable initializer type does not match global variable type!
 *	[2 x i64]* @global_array
 */

#include <stddef.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

#define TYPE_INT LLVMInt64Type()
#define TYPE_ARRAY LLVMArrayType(TYPE_INT, 2)

int main(void)
{
	LLVMModuleRef module;
	LLVMValueRef global_int, global_array;
	LLVMValueRef int_init, array_init;
	LLVMValueRef array_vals[2];

	module = LLVMModuleCreateWithName("example");

	global_int = LLVMAddGlobal(module, TYPE_INT, "global_int");
	global_array = LLVMAddGlobal(module, TYPE_ARRAY, "global_array");

	int_init = LLVMConstInt(TYPE_INT, 31, 0);
	LLVMSetInitializer(global_int, int_init);

	array_vals[0] = LLVMConstInt(TYPE_INT, 42, 0);
	array_vals[1] = LLVMConstPtrToInt(global_int, TYPE_INT);
	array_init = LLVMConstArray(TYPE_ARRAY, array_vals, 2);
	LLVMSetInitializer(global_array, array_init);

	LLVMDumpModule(module);
	LLVMVerifyModule(module, LLVMPrintMessageAction, NULL);
}
