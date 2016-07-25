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

	/* TODO: create/destroy symbol table for each function definition */
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

/* TODO: Store necessary globales in struct called "state" */
static LLVMBasicBlockRef mylabel = NULL;
static LLVMValueRef func;
static LLVMValueRef retval;

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

LLVMValueRef gen_simpledef(struct node *ast)
{
	/*
	 * TODO: Make a note that a "vector" in B terminology equates to
	 * an LLVM array, not an LLVM vector
	 */
	LLVMValueRef newdef, olddef, array, *elems;
	LLVMTypeRef type;
	struct node *ival_list;
	uint64_t i = 0, size = 0;

	ival_list = two(ast);
	while (ival_list) {
		size++;
		ival_list = two(ival_list);
	}

	elems = calloc(sizeof(LLVMValueRef), size);

	if (elems == NULL)
		generror("Out of memory");

	ival_list = two(ast);
	while (ival_list) {
		/* TODO: handle NAMES (convert global pointer to int) */
		elems[i++] = codegen(one(ival_list));
		ival_list = two(ival_list);
	}

	type = LLVMArrayType(TYPE_INT, size);
	array = LLVMConstArray(type, elems, size);

	newdef = LLVMAddGlobal(module, type, val(one(ast)));
	olddef = LLVMGetNamedGlobal(module, val(one(ast)));

	if (olddef != NULL) {
		LLVMReplaceAllUsesWith(olddef, newdef);
		LLVMDeleteGlobal(olddef);
		LLVMSetValueName(newdef, val(one(ast)));
	}

	LLVMSetInitializer(newdef, array);
	LLVMSetLinkage(newdef, LLVMExternalLinkage);

	return NULL;
}

LLVMValueRef gen_vecdef(struct node *ast)
{
	/*
	 * TODO: Make a note that a "vector" in B terminology equates to
	 * an LLVM array, not an LLVM vector
	 */
	LLVMValueRef newdef, olddef, array, *elems;
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
	if (elems == NULL)
		generror("Out of memory");

	ival_list = three(ast);
	while (ival_list) {
		/* TODO: handle NAMES (convert global pointer to int) */
		elems[i++] = codegen(one(ival_list));
		ival_list = two(ival_list);
	}

	while (i < minsize)
		elems[i++] = LLVMConstInt(TYPE_INT, 0, 0);

	/* TODO: Store actual size for reuse */
	type = LLVMArrayType(TYPE_INT, size >= minsize ? size : minsize);
	array = LLVMConstArray(type, elems, size >= minsize ? size : minsize);

	newdef = LLVMAddGlobal(module, type, val(one(ast)));
	olddef = LLVMGetNamedGlobal(module, val(one(ast)));

	if (olddef != NULL) {
		LLVMReplaceAllUsesWith(olddef, newdef);
		LLVMDeleteGlobal(olddef);
		LLVMSetValueName(newdef, val(one(ast)));
	}

	LLVMSetInitializer(newdef, array);
	LLVMSetLinkage(newdef, LLVMExternalLinkage);

	return NULL;
}

LLVMValueRef gen_expression(struct node *ast)
{
	return NULL;
}

LLVMValueRef gen_return(struct node *ast)
{
	LLVMBasicBlockRef next_block, ret_block;

	if (one(ast))
		LLVMBuildStore(builder, codegen(one(ast)), retval);

	ret_block = LLVMGetLastBasicBlock(func);
	LLVMBuildBr(builder, ret_block);

	next_block = LLVMInsertBasicBlock(ret_block, "");
	LLVMPositionBuilderAtEnd(builder, next_block);;

	return NULL;
}

LLVMValueRef gen_label(struct node *ast)
{
	/* TODO: Use separate table for labels */
	LLVMBasicBlockRef prev_block;

	prev_block = LLVMGetInsertBlock(builder);

	/* TODO: Use GetBasicBlockParent() instead of relying on global? */
	if (mylabel == NULL) {
		mylabel = LLVMAppendBasicBlock(func, val(one(ast)));
	} else {
		LLVMMoveBasicBlockAfter(mylabel, prev_block);
		/* TODO: Prefix label names to avoid clashes with do/then? */
		LLVMSetValueName((LLVMValueRef)mylabel, val(one(ast)));
	}

	LLVMPositionBuilderAtEnd(builder, prev_block);
	LLVMBuildBr(builder, mylabel);
	LLVMPositionBuilderAtEnd(builder, mylabel);
	codegen(two(ast));

	return NULL;
}

LLVMValueRef gen_goto(struct node *ast)
{
	LLVMBasicBlockRef next_block;
	/* TODO: check that NAME is LabelTypeKind in lvalue */
	/* or should labels be in different namespace? */
	if (mylabel == NULL)
		mylabel = LLVMAppendBasicBlock(func, "");

	LLVMBuildBr(builder, mylabel);

	next_block = LLVMAppendBasicBlock(func, "block");
	LLVMPositionBuilderAtEnd(builder, next_block);

	return NULL;
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
	LLVMBasicBlockRef do_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(one(ast)), zero, "tmp_cond");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	do_block = LLVMAppendBasicBlock(func, "do");
	end = LLVMAppendBasicBlock(func, "end");

	LLVMBuildCondBr(builder, condition, do_block, end);
	LLVMPositionBuilderAtEnd(builder, do_block);
	/* TODO: I don't think we need to collect values from then/else blocks */
	body_value = codegen(two(ast));

	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(one(ast)), zero, "tmp_cond");
	LLVMBuildCondBr(builder, condition, do_block, end);

	LLVMPositionBuilderAtEnd(builder, end);

	return body_value;
}

LLVMValueRef gen_if(struct node *ast)
{
	LLVMValueRef zero, condition, func, ref;
	LLVMBasicBlockRef then_block, else_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = codegen(one(ast));
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "tmp_cond");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(func, "then");
	else_block = LLVMAppendBasicBlock(func, "else");
	end = LLVMAppendBasicBlock(func, "end");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	codegen(two(ast));
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	if (three(ast))
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

LLVMValueRef gen_cond(struct node *ast)
{
	/* TODO: Rename to avoid conflict with IF condition */
	/* TODO: Figure out best way to convert to and from Int1 */
	return LLVMBuildSelect(builder,
		LLVMBuildICmp(builder,
			LLVMIntNE,
			codegen(one(ast)),
			LLVMConstInt(TYPE_INT, 0, 0),
			"tmp_not"),
		codegen(two(ast)),
		codegen(three(ast)),
		"tmp_cond");
}

LLVMValueRef gen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_add");
}

LLVMValueRef gen_mul(struct node *ast)
{
	return LLVMBuildMul(builder,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_mul");
}

LLVMValueRef gen_mod(struct node *ast)
{
	return LLVMBuildSRem(builder,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_mod");
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

	codegen(two(ast));

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
		case LLVMPointerTypeKind:
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

	/* TODO: Should extrns be added to symbol table to prevent accessing
	 * global variables that weren't declared? */
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

LLVMValueRef gen_div_assign(struct node *ast)
{
	LLVMValueRef result, left, right;

	left = codegen(one(ast));
	right = codegen(two(ast));

	if (!left || !right)
		return NULL;

	result = LLVMBuildSDiv(builder, left, right, "tmp_div");
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
	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return result;
}

LLVMValueRef gen_postinc(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(one(ast));

	/* TODO: Add macro for creating constants */
	result = LLVMBuildAdd(builder,
		orig,
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return orig;
}

LLVMValueRef gen_preinc(struct node *ast)
{
	LLVMValueRef result;

	/* TODO: Add macro for creating constants */
	result = LLVMBuildAdd(builder,
		codegen(one(ast)),
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return result;
}

LLVMValueRef gen_postdec(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(one(ast));

	/* TODO: Add macro for creating constants */
	result = LLVMBuildSub(builder,
		orig,
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return orig;
}

LLVMValueRef gen_predec(struct node *ast)
{
	LLVMValueRef result;

	/* TODO: Add macro for creating constants */
	result = LLVMBuildSub(builder,
		codegen(one(ast)),
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(one(ast)));

	return result;
}

LLVMValueRef gen_const(struct node *ast)
{
	/* TODO: Handle strings, multichars, escape sequences, and octal ints */
	if (val(ast)[0] == '"')
		return NULL;
	else if (strcmp("'*n'", val(ast)) == 0)
		return LLVMConstInt(TYPE_INT, '\n', 0);
	else if (val(ast)[0] == '\'')
		return LLVMConstInt(TYPE_INT, val(ast)[1], 0);
	else if (val(ast)[0] == '0')
		return LLVMConstIntOfString(TYPE_INT, val(ast), 8);
	else
		return LLVMConstIntOfString(TYPE_INT, val(ast), 10);
}


LLVMValueRef gen_statements(struct node *ast)
{
	codegen(one(ast));
	return codegen(two(ast));
}


LLVMValueRef gen_call(struct node *ast)
{
	LLVMValueRef global;
	LLVMValueRef arg;
	LLVMTypeRef sig, argtype;

	/* Rename global to func */
	global = LLVMGetNamedGlobal(module, val(one(ast)));

	if (global == NULL) {
		argtype = TYPE_INT;
		sig = LLVMFunctionType(TYPE_INT, &argtype, 1, 0);
		global = LLVMAddGlobal(module, sig, val(one(ast)));
		LLVMSetLinkage(global, LLVMExternalLinkage);
	}

	/* TODO: Check that existing global is a function with same # of args */

	/* TODO: support multiple args */
	arg = codegen(two(ast));

	return LLVMBuildCall(builder, global, &arg, 1, "tmp_call");
}

LLVMValueRef gen_extrn(struct node *ast)
{
	/*
	* also set up vector initialization.
	* TODO: Warn when using unitialized var
	* TODO: Determine type to use at runtime (or maybe always use Int64..
	* see "http://llvm.org/docs/GetElementPtr.html#how-is-gep-different-from-ptrtoint-arithmetic-and-inttoptr" -- LLVM assumes pointers are <= 64 bits
	* accept commandline argument or look at sizeof(void *)
	* TODO: Put global in symbol table
	*/

	LLVMValueRef extrn;

	struct node *name_list;
	char *name;

	name_list = one(ast);

	while (name_list) {
		name = val(one(name_list));

		if (LLVMGetNamedGlobal(module, name) == NULL) {
			/* TODO: Figure out what type should be used to allow both
			 * array and integer globals */
			//extrn = LLVMAddGlobal(module, TYPE_INT, name);
			//extrn = LLVMAddGlobal(module, LLVMArrayType(TYPE_INT, 2000), name);
			extrn = LLVMAddGlobal(module, LLVMPointerType(TYPE_INT, 0), name);
		}

		name_list = two(name_list);
	}

	codegen(two(ast));

	return NULL;
}

LLVMValueRef gen_funcdef(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMBasicBlockRef body_block, ret_block;

	/* TODO: Check if function already defined
	 * ? Can functions be looked up like globals ? */
	sig = LLVMFunctionType(TYPE_INT, NULL, 0, 0);
	func = LLVMAddFunction(module, val(one(ast)), sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	body_block = LLVMAppendBasicBlock(func, "");
	ret_block = LLVMAppendBasicBlock(func, "ret_block");
	LLVMPositionBuilderAtEnd(builder, body_block);

	retval = LLVMBuildAlloca(builder, TYPE_INT, "tmp_ret");
	LLVMBuildStore(builder, LLVMConstInt(TYPE_INT, 0, 0), retval);

	codegen(three(ast));
	LLVMBuildBr(builder, ret_block);

	/* TODO: Untangle out-of-order blocks */
	LLVMPositionBuilderAtEnd(builder, ret_block);
	LLVMBuildRet(builder, LLVMBuildLoad(builder, retval, "tmp_ret"));

	/* TODO: Handle failed verification and print internal compiler error */
	if (LLVMVerifyFunction(func, LLVMPrintMessageAction))
		LLVMDeleteFunction(func);

	return NULL;
}

LLVMValueRef gen_defs(struct node *ast)
{

	codegen(one(ast));
	return codegen(two(ast));
}

LLVMValueRef gen_not(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntEQ,
		codegen(one(ast)),
		LLVMConstInt(TYPE_INT, 0, 0),
		"tmp_not");
}

LLVMValueRef gen_ne(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntNE,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_ne");
}

LLVMValueRef gen_eq(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntEQ,
		codegen(one(ast)),
		codegen(two(ast)),
		"tmp_eq");
}

LLVMValueRef gen_and(struct node *ast) { generror("Not yet implemented: gen_and"); return NULL; }
LLVMValueRef gen_and_assign(struct node *ast) { generror("Not yet implemented: gen_and_assign"); return NULL; }
LLVMValueRef gen_args(struct node *ast) { generror("Not yet implemented: gen_args"); return NULL; }
LLVMValueRef gen_case(struct node *ast) { generror("Not yet implemented: gen_case"); return NULL; }
LLVMValueRef gen_comma(struct node *ast) { generror("Not yet implemented: gen_comma"); return NULL; }
LLVMValueRef gen_div(struct node *ast) { generror("Not yet implemented: gen_div"); return NULL; }
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
LLVMValueRef gen_mod_assign(struct node *ast) { generror("Not yet implemented: gen_mod_assign"); return NULL; }
LLVMValueRef gen_mul_assign(struct node *ast) { generror("Not yet implemented: gen_mul_assign"); return NULL; }
LLVMValueRef gen_names(struct node *ast) { generror("Not yet implemented: gen_names"); return NULL; }
LLVMValueRef gen_ne_assign(struct node *ast) { generror("Not yet implemented: gen_ne_assign"); return NULL; }
LLVMValueRef gen_neg(struct node *ast) { generror("Not yet implemented: gen_neg"); return NULL; }
LLVMValueRef gen_or_assign(struct node *ast) { generror("Not yet implemented: gen_or_assign"); return NULL; }
LLVMValueRef gen_right(struct node *ast) { generror("Not yet implemented: gen_right"); return NULL; }
LLVMValueRef gen_right_assign(struct node *ast) { generror("Not yet implemented: gen_right_assign"); return NULL; }
LLVMValueRef gen_sub(struct node *ast) { generror("Not yet implemented: gen_sub"); return NULL; }
LLVMValueRef gen_sub_assign(struct node *ast) { generror("Not yet implemented: gen_sub_assign"); return NULL; }
LLVMValueRef gen_switch(struct node *ast) { generror("Not yet implemented: gen_switch"); return NULL; }
LLVMValueRef gen_xor(struct node *ast) { generror("Not yet implemented: gen_xor"); return NULL; }
LLVMValueRef gen_xor_assign(struct node *ast) { generror("Not yet implemented: gen_xor_assign"); return NULL; }
