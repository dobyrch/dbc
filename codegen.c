#include <search.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "dbc.tab.h"
#include "astnode.h"
#include "codegen.h"

#define SYMTAB_SIZE 1024
#define TYPE_INT LLVMInt64Type()
#define TYPE_PTR LLVMPointerType(TYPE_INT, 0)


static LLVMBuilderRef builder;
static LLVMModuleRef module;


/* TODO: rename struct node to ast_node, rename arg ast to node */
void compile(struct node *ast)
{
	/* TODO: Free module, define "dbc" as constant */
	if ((module = LLVMModuleCreateWithName("dbc")) == NULL)
		generror("Failed to create LLVM module");

	if ((builder = LLVMCreateBuilder()) == NULL)
		generror("Failed to create LLVM instruction builder");

	if (hcreate(SYMTAB_SIZE) == 0)
		generror("Failed to allocate space for symbol table");


	/* TODO: Remove superfluous returns from gen_ */
	codegen(ast);
	printf("\n====================================\n");
	LLVMDumpModule(module);
	printf("====================================\n");

	if (LLVMWriteBitcodeToFile(module, "dbc.bc") != 0) {
		generror("Failed to write bitcode");
	}
}

void free_tree(struct node *ast)
{
	/* TODO:  Free while compiling instead? */
}

/* TODO: Separate functions for internal compiler error, code error, code warning, etc. */
/* TODO: Colorize output */
/* TODO: Look at clang/gcc for examples of error messages */
void generror(const char *msg, ...)
{
	va_list args;

	printf("ERROR: ");

	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);

	putchar('\n');

	exit(EXIT_FAILURE);
}


static LLVMBasicBlockRef mylabel = NULL;

LLVMValueRef gen_compound(struct node *ast)
{
	if (one(ast))
		return codegen(one(ast));

	return NULL;
}

LLVMValueRef gen_index(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "tmp_load");
}

LLVMValueRef gen_vecdef(struct node *ast)
{
		/*
		 * TODO: Make a note that a "vector" in B terminology equates to
		 * an LLVM array, not an LLVM vector
		 */
		LLVMValueRef global, array, *elems;
		LLVMTypeRef type;
		struct node *ival_list;
		uint64_t i = 0, size = 0, minsize = 0;

		ival_list = three(ast);
		while (ival_list) {
			size++;
			ival_list = two(ival_list);
		}

		if (two(ast))
			/*
			 * TODO: check that type is not string;
			 * use convenience function for handling
			 * chars and octal constants
			 * TODO: Check for invalid (negative) array size
			 */
			minsize = atoi(val(two(ast)));

		elems = calloc(sizeof(LLVMValueRef), size >= minsize ? size : minsize);
		/* TODO: Check all allocs for errors */
		if (!elems)
			generror("Out of memory");

		ival_list = three(ast);
		while (ival_list) {
			/* TODO: handle NAMES (convert global pointer to int) */
			elems[i++] = codegen(one(ival_list));
			ival_list = two(ival_list);
		}

		while (i < minsize)
			elems[i++] = LLVMConstInt(TYPE_INT, 0, 0);


		type = LLVMArrayType(TYPE_INT, size);
		/* TODO: Figure out why "foo[6];" has size of 0 */
		array = LLVMConstArray(type, elems, size >= minsize ? size : minsize);

		global = LLVMAddGlobal(module, type, val(one(ast)));
		LLVMSetInitializer(global, array);
		LLVMSetLinkage(global, LLVMExternalLinkage);

		return global;
}

LLVMValueRef gen_expression(struct node *ast)
{
	return NULL;
}

LLVMValueRef retval;
LLVMValueRef gen_return(struct node *ast)
{
	if (one(ast))
		LLVMBuildStore(
			builder,
			codegen(one(ast)),
			retval);

	/* TODO: Jump to end of function */

	return NULL;
}

LLVMValueRef gen_label(struct node *ast)
{
	LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	mylabel = LLVMAppendBasicBlock(parent, "label");
	return codegen(one(ast));
}

LLVMValueRef gen_goto(struct node *ast)
{
	return LLVMBuildBr(builder, mylabel);
}

LLVMValueRef gen_addr(struct node *ast)
{
	/* TODO: Function pointers? */
	/* TODO: Check that lvalue is actually an lvalue */
	return LLVMBuildPtrToInt(builder, lvalue(one(ast)), TYPE_INT, "tmp_addr");
}

LLVMValueRef gen_indir(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "tmp_load");
}

LLVMValueRef gen_while(struct node *ast)
{
	LLVMValueRef condition, zero, func, body_value;
	LLVMBasicBlockRef body_block, end;

	condition = codegen(one(ast));
	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "ifcond");
	/* TODO: func isn't always a func... rename to "block" */
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	body_block = LLVMAppendBasicBlock(func, "body");
	end = LLVMAppendBasicBlock(func, "end");

	LLVMBuildCondBr(builder, condition, body_block, end);
	LLVMPositionBuilderAtEnd(builder, body_block);
	/* TODO: I don't think we need to collect values from then/else blocks */
	body_value = codegen(two(ast));

	LLVMBuildBr(builder, end);
	/* TODO: What's the point of this? */
	body_block = LLVMGetInsertBlock(builder);

	LLVMPositionBuilderAtEnd(builder, end);

	return body_value;
}

LLVMValueRef gen_if(struct node *ast)
{
	LLVMValueRef zero, condition, parent, ref;
	LLVMBasicBlockRef then_block, else_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = codegen(one(ast));
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "tmp_cond");
	parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(parent, "then");
	else_block = LLVMAppendBasicBlock(parent, "else");
	end = LLVMAppendBasicBlock(parent, "end");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	codegen(two(ast));
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	codegen(three(ast));
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, end);
	return ref;
}

LLVMValueRef gen_lt(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSLT,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_lt");
}

LLVMValueRef gen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_add");
}

LLVMValueRef gen_auto(struct node *ast)
{
	/*
	* also set up vector initialization.
	* TODO: Warn when using unitialized var
	* TODO: Determine type to use at runtime (or maybe always use Int64..
	* see "http://llvm.org/docs/GetElementPtr.html#how-is-gep-different-from-ptrtoint-arithmetic-and-inttoptr" -- LLVM assumes pointers are <= 64 bits
	* accept commandline argument or look at sizeof(void *)
	*/

	struct node *init_list, *init;
	ENTRY symtab_entry;

	init_list = one(ast);

	while (init_list) {
		init = one(init_list);
		symtab_entry.key = val(one(init));
		symtab_entry.data = LLVMBuildAlloca(builder, TYPE_INT, symtab_entry.key);

		if (hsearch(symtab_entry, FIND) != NULL)
			generror("redefinition of '%s'", symtab_entry.key);

		if (hsearch(symtab_entry, ENTER) == NULL)
			generror("Symbol table full");

		if (two(init))
			LLVMBuildStore(builder, codegen(two(init)), symtab_entry.data);

		init_list = two(init_list);
	}

	return NULL;
}

LLVMValueRef gen_name(struct node *ast)
{
	LLVMValueRef ptr;
	LLVMTypeRef type;

	ptr = lvalue(ast);
	type = LLVMGetElementType(LLVMTypeOf(ptr));

	switch (LLVMGetTypeKind(type)) {
		case LLVMIntegerTypeKind:
			return LLVMBuildLoad(builder, ptr, val(ast));
		case LLVMArrayTypeKind:
			return LLVMBuildPtrToInt(builder, ptr, TYPE_INT, "tmp_addr");
		default:
			generror("Unexpected type '%s'", LLVMPrintTypeToString(type));
			return NULL;
	}
}

LLVMValueRef lvalue(struct node *ast)
{
	if (ast->codegen == gen_name)
		return lvalue_name(ast);
	else if (ast->codegen == gen_indir)
		return lvalue_indir(ast);
	else if (ast->codegen == gen_index)
		return lvalue_index(ast);
	else
		/* TODO: handle NULL in gen_addr/gen_assign and print "expected lvalue" */
		return NULL;
}

LLVMValueRef lvalue_name(struct node *ast)
{
	LLVMValueRef global;
	ENTRY symtab_lookup, *symtab_entry;

	symtab_lookup.key = val(ast);
	symtab_lookup.data = NULL;

	symtab_entry = hsearch(symtab_lookup, FIND);

	if (symtab_entry != NULL)
		return symtab_entry->data;

	global = LLVMGetNamedGlobal(module, val(ast));

	if (global != NULL)
		return global;

	generror("Use of undeclared identifier '%s'", val(ast));
	return NULL;
}

LLVMValueRef lvalue_indir(struct node *ast)
{
	return LLVMBuildIntToPtr(builder, codegen(one(ast)), TYPE_PTR, "tmp_indir");
}

LLVMValueRef lvalue_index(struct node *ast)
{
	LLVMValueRef ptr, index, gep;

	/*
	 * TODO: ensure x[y] == y[x] holds
	 */
	ptr = LLVMBuildIntToPtr(builder, codegen(one(ast)), TYPE_PTR, "tmp_ptr");
	index = codegen(two(ast));
	gep = LLVMBuildGEP(builder, ptr, &index, 1, "tmp_subscript");

	return gep;
}

LLVMValueRef gen_assign(struct node *ast)
{

	LLVMValueRef result;
	result = codegen(two(ast));
	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return result;
}

LLVMValueRef gen_add_assign(struct node *ast)
{
	LLVMValueRef result, left, right;

	left = codegen(one(ast));
	right = codegen(two(ast));

	if (!left || !right)
		return NULL;

	result = LLVMBuildAdd(builder, left, right, "tmp_add");
	LLVMBuildStore(builder, result, left);

	return result;
}

LLVMValueRef gen_postdec(struct node *ast)
{
	return NULL;
	/*
	LLVMValueRef result;
	result = LLVMBuildSub(builder,
		codegen(one(ast)),
		LLVMConstInt(TYPE_INT, 1, 0),
		"subtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
	*/
}

LLVMValueRef gen_predec(struct node *ast)
{
	return NULL;
	/*
	LLVMValueRef tmp = myvar;
	myvar = LLVMBuildSub(builder,
		codegen(one(ast)),
		LLVMConstInt(TYPE_INT, -1, 0),
		"subtmp");

	return tmp;
	*/
}

LLVMValueRef gen_const(struct node *ast)
{
	/* TODO: Handle strings, multichars, escape sequences, and octal ints */
	if (ast->one.val[0] == '"')
		return NULL;
	else if (ast->one.val[0] == '\'')
		return LLVMConstInt(TYPE_INT, ast->one.val[1], 0);
	else
		return LLVMConstIntOfString(TYPE_INT, ast->one.val, 10);
}


LLVMValueRef gen_statements(struct node *ast)
{
	codegen(one(ast));
	return codegen(two(ast));
}


LLVMValueRef gen_call(struct node *ast)
{
	static int first = 1;
	LLVMValueRef func, *args;

	LLVMTypeRef signew;
	LLVMValueRef funcnew;
	LLVMTypeRef intarg = TYPE_INT;

	signew = LLVMFunctionType(TYPE_INT, &intarg, 1, 0);
	if (first) {
		funcnew = LLVMAddGlobal(module, signew, val(one(ast)));
		LLVMSetLinkage(funcnew, LLVMExternalLinkage);
		first = 0;
	} else {
		funcnew = LLVMGetNamedGlobal(module, val(one(ast)));
	}
	//LLVMInsertIntoBuilder(builder, funcnew);
	//func = LLVMGetNamedGlobal(module, val(one(ast)));
	func = funcnew;

	if (func == NULL) {
		return NULL;
	}

	args = malloc(sizeof(LLVMValueRef));
	/* TODO: support multiple args */
	*args = codegen(two(ast));

	return LLVMBuildCall(builder, func, args, 1, "calltmp");
}

LLVMValueRef gen_extrn(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMValueRef func;

	sig = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
	func = LLVMAddGlobal(module, sig, "putchar");
	LLVMSetLinkage(func, LLVMExternalLinkage);
	return func;
}

LLVMValueRef gen_funcdef(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMValueRef func, body;
	LLVMBasicBlockRef block;

	/* TODO: Check if function already defined */
	sig = LLVMFunctionType(TYPE_INT, NULL, 0, 0);
	func = LLVMAddFunction(module, val(one(ast)), sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, block);

	retval = LLVMBuildAlloca(builder, TYPE_INT, "retval");
	LLVMBuildStore(builder,
		LLVMConstInt(TYPE_INT, 0, 0),
		retval);
	body = codegen(three(ast));

	LLVMBuildRet(builder, LLVMBuildLoad(builder, retval, "retval"));

	if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
		LLVMDeleteFunction(func);
		return NULL;
	}

	return func;
}

LLVMValueRef gen_defs(struct node *ast)
{

	codegen(one(ast));
	return codegen(two(ast));
}

LLVMValueRef gen_and(struct node *ast) { generror("Not yet implemented: gen_and"); return NULL; }
LLVMValueRef gen_and_assign(struct node *ast) { generror("Not yet implemented: gen_and_assign"); return NULL; }
LLVMValueRef gen_args(struct node *ast) { generror("Not yet implemented: gen_args"); return NULL; }
LLVMValueRef gen_case(struct node *ast) { generror("Not yet implemented: gen_case"); return NULL; }
LLVMValueRef gen_comma(struct node *ast) { generror("Not yet implemented: gen_comma"); return NULL; }
LLVMValueRef gen_cond(struct node *ast) { generror("Not yet implemented: gen_cond"); return NULL; }
LLVMValueRef gen_div(struct node *ast) { generror("Not yet implemented: gen_div"); return NULL; }
LLVMValueRef gen_div_assign(struct node *ast) { generror("Not yet implemented: gen_div_assign"); return NULL; }
LLVMValueRef gen_eq(struct node *ast) { generror("Not yet implemented: gen_eq"); return NULL; }
LLVMValueRef gen_eq_assign(struct node *ast) { generror("Not yet implemented: gen_eq_assign"); return NULL; }
LLVMValueRef gen_ge(struct node *ast) { generror("Not yet implemented: gen_ge"); return NULL; }
LLVMValueRef gen_gt(struct node *ast) { generror("Not yet implemented: gen_gt"); return NULL; }
LLVMValueRef gen_init(struct node *ast) { generror("Not yet implemented: gen_init"); return NULL; }
LLVMValueRef gen_inits(struct node *ast) { generror("Not yet implemented: gen_inits"); return NULL; }
LLVMValueRef gen_ior(struct node *ast) { generror("Not yet implemented: gen_ior"); return NULL; }
LLVMValueRef gen_ivals(struct node *ast) { generror("Not yet implemented: gen_ivals"); return NULL; }
LLVMValueRef gen_le(struct node *ast) { generror("Not yet implemented: gen_le"); return NULL; }
LLVMValueRef gen_left(struct node *ast) { generror("Not yet implemented: gen_left"); return NULL; }
LLVMValueRef gen_left_assign(struct node *ast) { generror("Not yet implemented: gen_left_assign"); return NULL; }
LLVMValueRef gen_mod(struct node *ast) { generror("Not yet implemented: gen_mod"); return NULL; }
LLVMValueRef gen_mod_assign(struct node *ast) { generror("Not yet implemented: gen_mod_assign"); return NULL; }
LLVMValueRef gen_mul(struct node *ast) { generror("Not yet implemented: gen_mul"); return NULL; }
LLVMValueRef gen_mul_assign(struct node *ast) { generror("Not yet implemented: gen_mul_assign"); return NULL; }
LLVMValueRef gen_names(struct node *ast) { generror("Not yet implemented: gen_names"); return NULL; }
LLVMValueRef gen_ne(struct node *ast) { generror("Not yet implemented: gen_ne"); return NULL; }
LLVMValueRef gen_ne_assign(struct node *ast) { generror("Not yet implemented: gen_ne_assign"); return NULL; }
LLVMValueRef gen_neg(struct node *ast) { generror("Not yet implemented: gen_neg"); return NULL; }
LLVMValueRef gen_not(struct node *ast) { generror("Not yet implemented: gen_not"); return NULL; }
LLVMValueRef gen_or_assign(struct node *ast) { generror("Not yet implemented: gen_or_assign"); return NULL; }
LLVMValueRef gen_postinc(struct node *ast) { generror("Not yet implemented: gen_postinc"); return NULL; }
LLVMValueRef gen_preinc(struct node *ast) { generror("Not yet implemented: gen_preinc"); return NULL; }
LLVMValueRef gen_right(struct node *ast) { generror("Not yet implemented: gen_right"); return NULL; }
LLVMValueRef gen_right_assign(struct node *ast) { generror("Not yet implemented: gen_right_assign"); return NULL; }
LLVMValueRef gen_simpledef(struct node *ast) { generror("Not yet implemented: gen_simpledef"); return NULL; }
LLVMValueRef gen_sub(struct node *ast) { generror("Not yet implemented: gen_sub"); return NULL; }
LLVMValueRef gen_sub_assign(struct node *ast) { generror("Not yet implemented: gen_sub_assign"); return NULL; }
LLVMValueRef gen_switch(struct node *ast) { generror("Not yet implemented: gen_switch"); return NULL; }
LLVMValueRef gen_xor(struct node *ast) { generror("Not yet implemented: gen_xor"); return NULL; }
LLVMValueRef gen_xor_assign(struct node *ast) { generror("Not yet implemented: gen_xor_assign"); return NULL; }
