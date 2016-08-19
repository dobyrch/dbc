/*
Running with

	clang -c example.c $(llvm-config --cflags)
	clang++ example.o -o example $(llvm-config --ldflags --libs --system-libs)
	./example

outputs

	; ModuleID = 'example'

	@array_one = global [1 x i64] [i64 42]
	@array_two = global [1 x i64] [[1 x i64] ptrtoint ([1 x i64]* @array_one to i64)]
	Global variable initializer type does not match global variable type!
	[1 x i64]* @array_two

Note that the output type of ptrtoint is [1 x i64], not i64.  If the
output is manually edited to change the type of ptrtoint to i64, llc
compiles it without error.
*/

#include <stddef.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

#define TYPE_INT LLVMInt64Type()
#define TYPE_ARRAY(n) LLVMArrayType(TYPE_INT, (n))

int main(void)
{
	LLVMModuleRef module;
	LLVMValueRef array_one, array_two;
	LLVMValueRef init_one, init_two;
	LLVMValueRef val_one, val_two;

	module = LLVMModuleCreateWithName("example");

	array_one = LLVMAddGlobal(module, TYPE_ARRAY(1), "array_one");
	array_two = LLVMAddGlobal(module, TYPE_ARRAY(1), "array_two");

	val_one = LLVMConstInt(TYPE_INT, 42, 0);
	init_one = LLVMConstArray(TYPE_ARRAY(1), &val_one, 1);
	LLVMSetInitializer(array_one, init_one);

	val_two = LLVMConstPtrToInt(array_one, TYPE_INT);
	init_two = LLVMConstArray(TYPE_ARRAY(1), &val_two, 1);
	LLVMSetInitializer(array_two, init_two);

	LLVMDumpModule(module);
	LLVMVerifyModule(module, LLVMPrintMessageAction, NULL);
}
