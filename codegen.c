#include <limits.h>
#include <search.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/Scalar.h>

#include "dbc.tab.h"
#include "astnode.h"
#include "codegen.h"

/* TODO: Dynamic memory allocation */
#define MAX_STRSIZE 1024
#define MAX_CHARSIZE 4
#define SYMTAB_SIZE 1024
#define MAX_LABELS 256
#define MAX_CASES 256
#define MAX_SWITCHES 64

#define TYPE_INT LLVMInt64Type()
#define TYPE_PTR LLVMPointerType(TYPE_INT, 0)
#define TYPE_FUNC LLVMFunctionType(TYPE_INT, NULL, 0, 1)
#define TYPE_LABEL LLVMPointerType(LLVMInt8Type(), 0)
#define TYPE_ARRAY(n) LLVMArrayType(TYPE_INT, (n))

#define CONST(n) LLVMConstInt(TYPE_INT, (n), 0)

static LLVMBuilderRef builder;
static LLVMModuleRef module;

/* TODO: Store necessary globales in struct called "state" */
static LLVMValueRef retval;
static LLVMBasicBlockRef ret_block;
static LLVMBasicBlockRef labels[MAX_LABELS];
static LLVMBasicBlockRef cases[MAX_SWITCHES][MAX_CASES];
static LLVMValueRef switches[MAX_SWITCHES];
static int case_count[MAX_SWITCHES];
static int label_count, switch_count, case_i;

/* TODO: Separate functions for internal compiler error, code error, code warning, etc. */
/* TODO: Colorize output */
/* TODO: Look at clang/gcc for examples of error messages */
static void generror(const char *msg, ...)
{
	va_list args;

	printf("ERROR: ");

	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);

	putchar('\n');

	exit(EXIT_FAILURE);
}

static void symtab_enter(char *key, void *data)
{

	ENTRY symtab_entry;

	symtab_entry.key = key;
	symtab_entry.data = data;

	if (hsearch(symtab_entry, FIND) != NULL)
		generror("redefinition of '%s'", symtab_entry.key);

	if (hsearch(symtab_entry, ENTER) == NULL)
		generror("Symbol table full");
}

static void *symtab_find(char *key)
{

	ENTRY symtab_lookup, *symtab_entry;

	symtab_lookup.key = key;
	symtab_lookup.data = NULL;

	symtab_entry = hsearch(symtab_lookup, FIND);

	return symtab_entry ? symtab_entry->data : NULL;
}

static int count_chain(struct node *ast)
{
	if (ast == NULL)
		return 0;

	return 1 + count_chain(ast->two);
}

void compile(struct node *ast)
{
	LLVMPassManagerRef pass_manager;

	/* TODO: Free module, define "dbc" as constant */
	if ((module = LLVMModuleCreateWithName("dbc")) == NULL)
		generror("Failed to create LLVM module");

	if ((builder = LLVMCreateBuilder()) == NULL)
		generror("Failed to create LLVM instruction builder");

	if (ast)
		codegen(ast);

	printf("\n====================================\n");
	LLVMDumpModule(module);
	printf("------------------------------------\n");
	LLVMVerifyModule(module, LLVMPrintMessageAction, NULL);
	printf("====================================\n");

	pass_manager = LLVMCreatePassManager();
	LLVMAddBasicAliasAnalysisPass(pass_manager);
	LLVMAddInstructionCombiningPass(pass_manager);
	LLVMAddReassociatePass(pass_manager);
	LLVMAddGVNPass(pass_manager);
	LLVMAddCFGSimplificationPass(pass_manager);
	LLVMRunPassManager(pass_manager, module);

	if (LLVMWriteBitcodeToFile(module, "dbc.bc") != 0) {
		generror("Failed to write bitcode");
	}
}

void free_tree(struct node *ast)
{
	/* TODO: Free while compiling instead? */
}

static void predeclare_funcdef(struct node *ast)
{
	LLVMValueRef func;
	LLVMTypeRef func_type, *param_types;
	int param_count, i;

	param_count = count_chain(ast->two);
	param_types = calloc(sizeof(LLVMTypeRef), param_count);

	if (param_count > 0 && param_types == NULL)
		generror("out of memory");

	for (i = 0; i < param_count; i++)
		param_types[i] = TYPE_INT;

	func_type = LLVMFunctionType(TYPE_INT, param_types, param_count, 0);

	func = LLVMAddFunction(module, ast->one->val, func_type);
	LLVMSetLinkage(func, LLVMExternalLinkage);
}

static void predeclare_simpledef(struct node *ast)
{
	LLVMValueRef global;

	global = LLVMAddGlobal(module, TYPE_INT, ast->one->val);
	LLVMSetLinkage(global, LLVMExternalLinkage);
}

static void predeclare_vecdef(struct node *ast)
{
	LLVMValueRef global;
	int size, initsize;

	initsize = count_chain(ast->three);
	size = ast->two ? atol(ast->two->val) : 0;
	/*
	 * TODO: check that type is not string;
	 * use convenience function for handling
	 * chars and octal constants
	 * TODO: Check for invalid (negative) array size
	 */

	if (initsize > size)
		size = initsize;

	global = LLVMAddGlobal(module, TYPE_ARRAY(size), ast->one->val);
	LLVMSetLinkage(global, LLVMExternalLinkage);
}

static void predeclare_defs(struct node *ast)
{
	/* TODO: Check for duplicated names */
	if (ast->one->codegen == gen_funcdef)
		predeclare_funcdef(ast->one);

	else if (ast->one->codegen == gen_simpledef)
		predeclare_simpledef(ast->one);

	else if (ast->one->codegen == gen_vecdef)
		predeclare_vecdef(ast->one);

	else
		generror("Unexpected definition type");

	if (ast->two)
		predeclare_defs(ast->two);
}

LLVMValueRef gen_defs(struct node *ast)
{
	static int first_time = 1;

	if (first_time) {
		predeclare_defs(ast);
		first_time = 0;
	}

	codegen(ast->one);

	if (ast->two)
		codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_simpledef(struct node *ast)
{
	LLVMValueRef global;

	global = LLVMGetNamedGlobal(module, ast->one->val);
	LLVMSetInitializer(global, codegen(ast->two));

	return NULL;
}

LLVMValueRef gen_vecdef(struct node *ast)
{
	LLVMValueRef global, init, *ival_list;
	struct node *n;
	int size, initsize, i;

	initsize = count_chain(ast->three);
	size = ast->two ? atol(ast->two->val) : 0;

	if (initsize > size)
		size = initsize;

	ival_list = calloc(sizeof(LLVMValueRef), size);

	if (size > 0 && ival_list == NULL)
		generror("out of memory");

	for (i = 0, n = ast->three; i < initsize; i++, n = n->two)
		/* TODO: handle NAMES (convert global pointer to int) */
		ival_list[i] = codegen(n->one);

	for (i = initsize; i < size; i++)
		ival_list[i] = CONST(0);

	global = LLVMGetNamedGlobal(module, ast->one->val);
	init = LLVMConstArray(TYPE_ARRAY(size), ival_list, size);
	LLVMSetInitializer(global, init);

	return NULL;
}

static void predeclare_labels(struct node *ast)
{
	LLVMBasicBlockRef label_block;
	LLVMValueRef func;
	char *name;

	if (ast->one)
		predeclare_labels(ast->one);

	if (ast->two)
		predeclare_labels(ast->two);

	if (ast->three)
		predeclare_labels(ast->three);

	if (ast->codegen == gen_label) {
		name = ast->one->val;

		/* TODO: Prefix label names to avoid clashes with do/then? */
		func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
		label_block = LLVMAppendBasicBlock(func, name);
		//symtab_enter(name, LLVMBlockAddress(func, label_block));
		symtab_enter(name, label_block);

		if (label_count >= MAX_LABELS)
			generror("Label table overflow");

		labels[label_count++] = label_block;
	}
}

static void predeclare_switches(struct node *ast)
{
	LLVMValueRef func;

	if (ast->one)
		predeclare_switches(ast->one);

	if (ast->two)
		predeclare_switches(ast->two);

	/* TODO: Will ast->three ever exist in this context? */
	if (ast->three)
		predeclare_switches(ast->three);

	if (ast->codegen == gen_case) {
		/*
		if (switch_count == 0)
			generror("Warning: 'case' statement not in switch statement");
		*/

		func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
		cases[switch_count][case_count[switch_count]] = LLVMAppendBasicBlock(func, "");
		case_count[switch_count]++;

		if (case_count[switch_count] >= MAX_CASES)
			generror("Maximum number of cases exceeded");

	} else if (ast->codegen == gen_switch) {
		switch_count++;

		if (switch_count >= MAX_SWITCHES)
			generror("Maximum number of switches exceeded");
	}
}

LLVMValueRef gen_funcdef(struct node *ast)
{
	LLVMValueRef func;
	LLVMBasicBlockRef body_block;

	if (hcreate(SYMTAB_SIZE) == 0)
		generror("Failed to allocate space for symbol table");

	func = LLVMGetNamedFunction(module, ast->one->val);

	body_block = LLVMAppendBasicBlock(func, "");
	ret_block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, body_block);

	retval = LLVMBuildAlloca(builder, TYPE_INT, "");
	LLVMBuildStore(builder, CONST(0), retval);

	label_count = 0;
	predeclare_labels(ast->three);

	switch_count = 0;
	predeclare_switches(ast->three);
	switch_count = 0;

	if (ast->two)
		codegen(ast->two);

	codegen(ast->three);

	LLVMBuildBr(builder, ret_block);
	/* TODO: Untangle out-of-order blocks */
	LLVMPositionBuilderAtEnd(builder, ret_block);
	LLVMBuildRet(builder, LLVMBuildLoad(builder, retval, ""));

	LLVMMoveBasicBlockAfter(ret_block, LLVMGetLastBasicBlock(func));

	/* TODO: Handle failed verification and print internal compiler error */
	LLVMVerifyFunction(func, LLVMPrintMessageAction);

	hdestroy();

	return NULL;
}

LLVMValueRef gen_ivals(struct node *ast)
{
	LLVMValueRef *ival_list;
	int size, i;

	size = count_chain(ast);
	ival_list = calloc(sizeof(LLVMValueRef), size);

	if (size > 0 && ival_list == NULL)
		generror("out of memory");

	for (i = 0; i < size; i++, ast = ast->two)
		ival_list[i] = codegen(ast->one);

	return LLVMConstArray(TYPE_ARRAY(size), ival_list, size);
}

LLVMValueRef gen_names(struct node *ast)
{
	LLVMValueRef func, pam, var;
	char *name;

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	/* Pam param, pam pam param... */
	for (pam = LLVMGetFirstParam(func); pam; pam = LLVMGetNextParam(pam))
	{
		name = ast->one->val;
		var = LLVMBuildAlloca(builder, TYPE_INT, name);
		symtab_enter(name, var);

		LLVMBuildStore(builder, pam, var);

		ast = ast->two;
	}

	return NULL;
}

LLVMValueRef gen_statements(struct node *ast)
{
	codegen(ast->one);
	return codegen(ast->two);
}

LLVMValueRef gen_auto(struct node *ast)
{
	codegen(ast->one);
	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_extrn(struct node *ast)
{
	/* TODO: Warn when using unitialized var */
	LLVMValueRef global;
	struct node *name_list;
	char *name;

	name_list = ast->one;

	while (name_list) {
		name = name_list->one->val;

		global = LLVMGetNamedFunction(module, name);

		if (global == NULL)
			global = LLVMGetNamedGlobal(module, name);

		if (global == NULL)
			global = LLVMAddGlobal(module, TYPE_FUNC, name);

		symtab_enter(name, global);

		name_list = name_list->two;
	}

	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_label(struct node *ast)
{
	/* TODO: Use separate table for labels? */
	LLVMBasicBlockRef label_block, prev_block;

	prev_block = LLVMGetInsertBlock(builder);
	label_block = symtab_find(ast->one->val);

	LLVMMoveBasicBlockAfter(label_block, prev_block);
	LLVMPositionBuilderAtEnd(builder, prev_block);
	LLVMBuildBr(builder, label_block);
	LLVMPositionBuilderAtEnd(builder, label_block);

	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_case(struct node *ast)
{
	LLVMBasicBlockRef case_block, prev_block;

	prev_block = LLVMGetInsertBlock(builder);
	case_block = cases[switch_count][case_i];
	LLVMAddCase(switches[switch_count], codegen(ast->one), case_block);

	LLVMMoveBasicBlockAfter(case_block, prev_block);
	if (case_i > 0)
		LLVMBuildBr(builder, case_block);
	LLVMPositionBuilderAtEnd(builder, case_block);

	case_i++;
	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_compound(struct node *ast)
{
	if (ast->one)
		return codegen(ast->one);

	return NULL;
}

LLVMValueRef gen_if(struct node *ast)
{
	LLVMValueRef condition, func;
	LLVMBasicBlockRef then_block, else_block, end;

	condition = codegen(ast->one);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, CONST(0), "");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(func, "");
	else_block = LLVMAppendBasicBlock(func, "");
	end = LLVMAppendBasicBlock(func, "");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	codegen(ast->two);
	LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	if (ast->three)
		codegen(ast->three);
	LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, end);

	return NULL;
}

LLVMValueRef gen_while(struct node *ast)
{
	LLVMValueRef condition, func;
	LLVMBasicBlockRef do_block, end;

	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(ast->one), CONST(0), "");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	do_block = LLVMAppendBasicBlock(func, "");
	end = LLVMAppendBasicBlock(func, "");

	LLVMBuildCondBr(builder, condition, do_block, end);
	LLVMPositionBuilderAtEnd(builder, do_block);
	codegen(ast->two);

	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(ast->one), CONST(0), "");
	LLVMBuildCondBr(builder, condition, do_block, end);

	LLVMPositionBuilderAtEnd(builder, end);

	return NULL;
}

LLVMValueRef gen_switch(struct node *ast)
{
	LLVMValueRef func;
	LLVMBasicBlockRef next_block;

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	next_block = LLVMAppendBasicBlock(func, "");

	switches[switch_count] = LLVMBuildSwitch(builder, codegen(ast->one), next_block, case_count[switch_count]);
	case_i = 0;
	codegen(ast->two);

	LLVMPositionBuilderAtEnd(builder, cases[switch_count][case_count[switch_count]-1]);
	LLVMBuildBr(builder, next_block);
	LLVMPositionBuilderAtEnd(builder, next_block);
	switch_count++;

	return NULL;
}

LLVMValueRef gen_goto(struct node *ast)
{
	LLVMValueRef branch, func;
	LLVMBasicBlockRef next_block;
	int i;

	branch = LLVMBuildIndirectBr(builder,
			LLVMBuildIntToPtr(builder,
				codegen(ast->one),
				TYPE_LABEL,
				""),
			label_count);

	for (i = 0; i < label_count; i++)
		LLVMAddDestination(branch, labels[i]);

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	next_block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, next_block);

	return NULL;
}

LLVMValueRef gen_return(struct node *ast)
{
	LLVMBasicBlockRef next_block;

	if (ast->one)
		LLVMBuildStore(builder, codegen(ast->one), retval);

	LLVMBuildBr(builder, ret_block);

	next_block = LLVMInsertBasicBlock(ret_block, "");
	LLVMPositionBuilderAtEnd(builder, next_block);;

	return NULL;
}

LLVMValueRef gen_null(struct node *ast)
{
	return NULL;
}

LLVMValueRef gen_inits(struct node *ast)
{
	codegen(ast->one);

	if (ast->two)
		codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_init(struct node *ast)
{
	LLVMTypeRef type;
	LLVMValueRef var;
	char *name;

	name = ast->one->val;
	/* TODO: Warn when using unitialized var */
	/* TODO: replaces calls to atol with function that gets
	 * value of any constant literal */
	type = ast->two ? TYPE_ARRAY(atol(ast->two->val)) : TYPE_INT;
	var = LLVMBuildAlloca(builder, type, name);

	symtab_enter(name, var);

	return NULL;
}

static LLVMValueRef lvalue_name(struct node *ast)
{
	LLVMValueRef lvalue;

	lvalue = symtab_find(ast->val);

	/* TODO: Allow undeclared functions for calls */
	if (lvalue == NULL)
		generror("Use of undeclared identifier '%s'", ast->val);

	return lvalue;
}

static LLVMValueRef lvalue_indir(struct node *ast)
{
	return LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, "");
}

static LLVMValueRef lvalue_index(struct node *ast)
{
	LLVMValueRef ptr, index, gep;

	/*
	 * TODO: ensure x[y] == y[x] holds
	 */
	ptr = LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, "");
	index = codegen(ast->two);
	gep = LLVMBuildGEP(builder, ptr, &index, 1, "");

	return gep;
}

static LLVMValueRef lvalue(struct node *ast)
{
	if (ast->codegen == gen_name)
		return lvalue_name(ast);
	else if (ast->codegen == gen_indir)
		return lvalue_indir(ast);
	else if (ast->codegen == gen_index)
		return lvalue_index(ast);
	else
		generror("expected lvalue");

	return NULL;
}

LLVMValueRef gen_assign(struct node *ast)
{

	LLVMValueRef result;

	result = codegen(ast->two);
	/* TODO: Forbid assignment to labels */
	/* TODO: Allow assignment to arrays (cast result to type of lvalue)*/
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_mul_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_mul(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_div_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_div(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_mod_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_mod(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_add_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_add(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_sub_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_sub(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_left_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_left(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_right_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_right(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_and_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_and(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_or_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_or(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_eq_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_eq(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_ne_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_ne(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_lt_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_lt(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_le_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_le(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_gt_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_gt(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_ge_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_ge(ast);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_cond(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntNE,
			codegen(ast->one),
			CONST(0),
			"");

	return LLVMBuildSelect(builder,
			truth,
			codegen(ast->two),
			codegen(ast->three),
			"");
}

LLVMValueRef gen_or(struct node *ast)
{
	return LLVMBuildOr(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_and(struct node *ast)
{
	return LLVMBuildAnd(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_eq(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntEQ,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_ne(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntNE,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_lt(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntSLT,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_le(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntSLT,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_gt(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntSGT,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_ge(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntSGE,
			codegen(ast->one),
			codegen(ast->two),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_left(struct node *ast)
{
	return LLVMBuildShl(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_right(struct node *ast)
{
	return LLVMBuildLShr(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_sub(struct node *ast)
{
	return LLVMBuildSub(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_mul(struct node *ast)
{
	return LLVMBuildMul(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_div(struct node *ast)
{
	return LLVMBuildSDiv(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_mod(struct node *ast)
{
	return LLVMBuildSRem(builder,
			codegen(ast->one),
			codegen(ast->two),
			"");
}

LLVMValueRef gen_indir(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "");
}

LLVMValueRef gen_addr(struct node *ast)
{
	/* TODO: Function pointers? */
	/* TODO: Check that lvalue is actually an lvalue */
	return LLVMBuildPtrToInt(builder, lvalue(ast->one), TYPE_INT, "");
}

LLVMValueRef gen_neg(struct node *ast)
{
	return LLVMBuildNeg(builder,
			codegen(ast->one),
			"");
}

LLVMValueRef gen_not(struct node *ast)
{
	LLVMValueRef truth;

	truth = LLVMBuildICmp(builder,
			LLVMIntEQ,
			codegen(ast->one),
			CONST(0),
			"");

	return LLVMBuildZExt(builder, truth, TYPE_INT, "");
}

LLVMValueRef gen_preinc(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildAdd(builder,
			codegen(ast->one),
			CONST(1),
			"");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_postinc(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(ast->one);

	result = LLVMBuildAdd(builder,
			orig,
			CONST(1),
			"");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_predec(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildSub(builder,
			codegen(ast->one),
			CONST(1),
			"");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_postdec(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(ast->one);

	result = LLVMBuildSub(builder,
			orig,
			CONST(1),
			"");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_index(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "");
}

LLVMValueRef gen_call(struct node *ast)
{
	/* TODO: Check that existing global is a function with same # of args */
	/* TODO: Standardize variable naming between gen_call and gen_*def */
	LLVMValueRef func, *arg_list = NULL;
	struct node *n;
	int arg_count, i;

	func = LLVMBuildBitCast(builder,
			LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, ""),
			LLVMPointerType(TYPE_FUNC, 0),
			"");

	arg_count = count_chain(ast->two);
	arg_list = calloc(sizeof(LLVMValueRef), arg_count);

	if (arg_count > 0 && arg_list == NULL)
		generror("out of memory");

	for (i = arg_count - 1, n = ast->two; i >= 0; i--, n = n->two)
		arg_list[i] = codegen(n->one);

	return LLVMBuildCall(builder, func, arg_list, arg_count, "");
}

LLVMValueRef gen_name(struct node *ast)
{
	LLVMValueRef func, ptr;
	LLVMTypeRef type;

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	ptr = lvalue(ast);
	type = LLVMTypeOf(ptr);

	if (LLVMGetTypeKind(type) == LLVMLabelTypeKind)
		return LLVMBuildPtrToInt(builder,
				LLVMBlockAddress(func, (LLVMBasicBlockRef)ptr),
				TYPE_INT,
				"");

	type = LLVMGetElementType(LLVMTypeOf(ptr));

	switch (LLVMGetTypeKind(type)) {
		case LLVMIntegerTypeKind:
			return LLVMBuildLoad(builder, ptr, ast->val);
		case LLVMArrayTypeKind:
		case LLVMFunctionTypeKind:
			return LLVMBuildPtrToInt(builder, ptr, TYPE_INT, "");
		default:
			generror("Unexpected type '%s'", LLVMPrintTypeToString(type));
			return NULL;
	}
}

static char escape(char c)
{
	switch (c) {
	case '0':
		return '\0';
	case 'e':
		return EOF;
	case '(':
		return '{';
	case ')':
		return '}';
	case 't':
		return '\t';
	case 'n':
		return '\n';
	case '*':
	case '\'':
	case '"':
		return c;
	default:
		generror("warning: unknown escape sequence '*%c'", c);
		return c;
	}
}

static long long pack_char(const char **str)
{

	long long intval = 0;
	int i, size = 0;
	char c, buf[MAX_CHARSIZE];
	const char *p;

	p = *str;

	while (p[1] != '\0' && size < MAX_CHARSIZE) {
		c = p[0];

		if (c == '*') {
			c = escape(p[1]);
			p++;
		}

		buf[size++] = c;
		p++;
	}

	if (p[1] == '\0')
		*str = NULL;
	else
		*str = p;

	for (i = 0; i < size; i++)
		intval |= buf[i] << CHAR_BIT*i;

	return intval;
}

static LLVMValueRef make_char(const char *str)
{
	LLVMValueRef charval;

	/* Skip leading ' */
	str += 1;
	charval = CONST(pack_char(&str));

	if (str)
		generror("warning: character constant too long");

	return charval;
}

static LLVMValueRef make_str(const char *str)
{
	LLVMValueRef global, strval, chars[MAX_STRSIZE + 1];
	int size = 0;

	global = LLVMGetNamedGlobal(module, str);

	if (global)
		return global;

	global = LLVMAddGlobal(module, TYPE_ARRAY(size), str);

	/* Skip leading " */
	str += 1;
	while (str && size < MAX_STRSIZE)
		chars[size++] = CONST(pack_char(&str));

	if (str)
		generror("warning: string constant too long");

	chars[size++] = CONST((unsigned char)EOF);

	strval = LLVMConstArray(TYPE_ARRAY(size), chars, size);
	LLVMSetInitializer(global, strval);
	LLVMSetLinkage(global, LLVMPrivateLinkage);

	return global;
}

LLVMValueRef gen_const(struct node *ast)
{
	switch (ast->val[0]) {
	case '\'':
		return make_char(ast->val);
	case '"':
		return make_str(ast->val);
	case '0':
		return LLVMConstIntOfString(TYPE_INT, ast->val, 8);
	default:
		return LLVMConstIntOfString(TYPE_INT, ast->val, 10);
	}
}
