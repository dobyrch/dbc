/*
 * Copyright 2016 Douglas Christman
 *
 * This file is part of dbc.
 *
 * dbc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits.h>
#include <search.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "y.tab.h"
#include "astnode.h"
#include "codegen.h"
#include "constants.h"
#include "platform.h"

#define SYMTAB_SIZE 1024
#define MAX_LABELS 256
#define MAX_CASES 256

#define TYPE_PTR LLVMPointerType(TYPE_INT, 0)
#define TYPE_FUNC LLVMFunctionType(TYPE_INT, NULL, 0, 1)
#define TYPE_LABEL LLVMPointerType(LLVMInt8Type(), 0)
#define TYPE_ARRAY(n) LLVMArrayType(TYPE_INT, (n))

#define CONST(n) LLVMConstInt(TYPE_INT, (n), 0)

static LLVMBuilderRef builder;
static LLVMModuleRef module;

static LLVMValueRef case_vals[MAX_CASES];
static LLVMBasicBlockRef case_blocks[MAX_CASES];
static LLVMBasicBlockRef label_blocks[MAX_LABELS];
static int case_count, label_count;

/* TODO: Try to continue if error is not fatal */
/* TODO: Create separate func without line num */
static void generror(const char *msg, ...)
{
	va_list args;

	/* TODO: Print correct line number */
	fprintf(stderr, "%d: ", yylineno);

	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);

	putchar('\n');

	exit(EXIT_FAILURE);
}

static int count_chain(struct node *ast)
{
	if (ast == NULL)
		return 0;

	return 1 + count_chain(ast->two);
}

static void symtab_enter(const char *key, void *data)
{

	ENTRY symtab_entry;

	symtab_entry.key = (char *)key;
	symtab_entry.data = data;

	if (hsearch(symtab_entry, FIND) != NULL)
		generror("rd %s", symtab_entry.key);

	if (hsearch(symtab_entry, ENTER) == NULL)
		generror(">s");
}

static void *symtab_find(const char *key)
{

	ENTRY symtab_lookup, *symtab_entry;

	symtab_lookup.key = (char *)key;
	symtab_lookup.data = NULL;

	symtab_entry = hsearch(symtab_lookup, FIND);

	return symtab_entry ? symtab_entry->data : NULL;
}

static void *find_or_add_global(const char *name)
{
	LLVMValueRef global;

	global = LLVMGetNamedGlobal(module, name);

	if (global == NULL)
		global = LLVMAddGlobal(module, TYPE_INT, name);

	return global;
}

static void check_store(LLVMValueRef rvalue, LLVMValueRef lvalue)
{
	if (LLVMIsAGlobalValue(lvalue))
		rvalue = LLVMBuildShl(builder, rvalue, CONST(WORDPOW), "");

	LLVMBuildStore(builder, rvalue, lvalue);
}

static LLVMValueRef rvalue_to_lvalue(LLVMValueRef rvalue)
{
	rvalue = LLVMBuildShl(builder, rvalue, CONST(WORDPOW), "");

	return LLVMBuildIntToPtr(builder, rvalue, TYPE_PTR, "");
}

static LLVMValueRef lvalue_to_rvalue(LLVMValueRef lvalue)
{
	/*
	* TODO: Make sure all addresses are word-aligned
	*       (autos, vectors, strings, etc.)
	*/
	lvalue =  LLVMBuildPtrToInt(builder, lvalue, TYPE_INT, "");

	return LLVMBuildLShr(builder, lvalue, CONST(WORDPOW), "");
}

void compile(struct node *ast, const char *outfile)
{
	/* TODO: Free module, define "dbc" as constant */
	if ((module = LLVMModuleCreateWithName("dbc")) == NULL)
		generror("Failed to create LLVM module");

	if ((builder = LLVMCreateBuilder()) == NULL)
		generror("Failed to create LLVM instruction builder");

	if (ast)
		codegen(ast);

	if (LLVMVerifyModule(module, LLVMPrintMessageAction, NULL) != 0) {
		fprintf(stderr, "\nCongratulations, you've found a bug!\n"
			"Please submit your program to "
			"<https://github.com/dobyrch/dbc/issues>\n");
		exit(EXIT_FAILURE);
	}

	if (LLVMWriteBitcodeToFile(module, outfile) != 0)
		generror("Failed to write bitcode");
}

void free_tree(struct node *ast)
{
	/* TODO: Free while compiling instead? */
}

LLVMValueRef gen_defs(struct node *ast)
{
	codegen(ast->one);

	if (ast->two)
		codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_simpledef(struct node *ast)
{
	LLVMValueRef global, init;

	global = find_or_add_global(ast->one->val);
	init = LLVMConstShl(codegen(ast->two), CONST(WORDPOW));

	LLVMSetInitializer(global, init);

	return NULL;
}

LLVMValueRef gen_vecdef(struct node *ast)
{
	LLVMValueRef global, array, init, *ival_list;
	struct node *n;
	int size, initsize, i;

	initsize = count_chain(ast->three);

	if (ast->two)
		size = LLVMConstIntGetZExtValue(codegen(ast->two));
	else
		size = 0;

	if (initsize > size)
		size = initsize;

	ival_list = calloc(sizeof(LLVMValueRef), size);

	if (size > 0 && ival_list == NULL)
		generror("out of memory");

	for (i = 0, n = ast->three; i < initsize; i++, n = n->two)
		/* TODO: handle NAMES (convert global pointer to int) */
		ival_list[initsize - i - 1] = codegen(n->one);

	for (i = initsize; i < size; i++)
		ival_list[i] = CONST(0);

	global = find_or_add_global(ast->one->val);
	array = LLVMAddGlobal(module, TYPE_ARRAY(size), ".gvec");
	LLVMSetLinkage(array, LLVMPrivateLinkage);

	if (initsize)
		init = LLVMConstArray(TYPE_ARRAY(size), ival_list, size);
	else
		init = LLVMConstNull(TYPE_ARRAY(size));

	LLVMSetInitializer(array, init);
	LLVMSetInitializer(global, LLVMBuildPtrToInt(builder, array, TYPE_INT, ""));

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

		func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
		label_block = LLVMAppendBasicBlock(func, name);
		symtab_enter(name, label_block);

		if (label_count >= MAX_LABELS)
			generror(">i");

		label_blocks[label_count++] = label_block;
	}
}

LLVMValueRef gen_funcdef(struct node *ast)
{
	LLVMValueRef global, func, retval;
	LLVMTypeRef func_type, *param_types;
	LLVMBasicBlockRef body_block, ret_block;
	int param_count, i;

	if (hcreate(SYMTAB_SIZE) == 0)
		generror(">s");

	param_count = count_chain(ast->two);
	param_types = calloc(sizeof(LLVMTypeRef), param_count);

	if (param_count > 0 && param_types == NULL)
		generror("out of memory");

	for (i = 0; i < param_count; i++)
		param_types[i] = TYPE_INT;

	func_type = LLVMFunctionType(TYPE_INT, param_types, param_count, 0);
	func = LLVMAddFunction(module, ".gfunc", func_type);
	LLVMSetLinkage(func, LLVMPrivateLinkage);
	/* TODO: How to specify stack alignment? Should be 16 bytes */
	LLVMAddFunctionAttr(func, LLVMStackAlignment);

	global = find_or_add_global(ast->one->val);
	LLVMSetInitializer(global, LLVMBuildPtrToInt(builder, func, TYPE_INT, ""));

	body_block = LLVMAppendBasicBlock(func, "");
	ret_block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, body_block);

	retval = LLVMBuildAlloca(builder, TYPE_INT, "");
	LLVMBuildStore(builder, CONST(0), retval);

	symtab_enter(ast->one->val, global);
	symtab_enter(".return", ret_block);
	symtab_enter(".retval", retval);

	label_count = 0;
	predeclare_labels(ast->three);

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

	/* TODO: Figure out how to initialize int with vector without error */
	/*return LLVMConstArray(TYPE_ARRAY(size), ival_list, size);*/
	return size ? ival_list[0] : CONST(0);
}

LLVMValueRef gen_names(struct node *ast)
{
	LLVMValueRef func, pam, var;
	char *name;

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	/* Pam param, pam pam param... */
	for (pam = LLVMGetLastParam(func); pam; pam = LLVMGetPreviousParam(pam))
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
	LLVMValueRef global;
	struct node *name_list;
	char *name;

	name_list = ast->one;

	while (name_list) {
		name = name_list->one->val;
		global = find_or_add_global(name);
		symtab_enter(name, global);

		name_list = name_list->two;
	}

	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_label(struct node *ast)
{
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
	LLVMValueRef func;
	LLVMBasicBlockRef this_block, next_block;

	this_block = LLVMGetInsertBlock(builder);
	func = LLVMGetBasicBlockParent(this_block);
	next_block = LLVMAppendBasicBlock(func, "");
	LLVMMoveBasicBlockAfter(next_block, this_block);

	case_blocks[case_count] = next_block;
	case_vals[case_count] = codegen(ast->one);

	LLVMBuildBr(builder, next_block);
	LLVMPositionBuilderAtEnd(builder, next_block);

	case_count++;

	if (case_count >= MAX_CASES)
		generror(">c");

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
	LLVMValueRef func, switch_;
	LLVMBasicBlockRef this_block, switch_first_block, switch_last_block, end_block;
	int i;

	this_block = LLVMGetInsertBlock(builder);
	func = LLVMGetBasicBlockParent(this_block);
	switch_first_block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, switch_first_block);

	case_count = 0;
	codegen(ast->two);

	switch_last_block = LLVMGetLastBasicBlock(func);
	end_block = LLVMAppendBasicBlock(func, "");

	LLVMPositionBuilderAtEnd(builder, switch_last_block);
	LLVMBuildBr(builder, end_block);

	LLVMPositionBuilderAtEnd(builder, this_block);
	switch_ = LLVMBuildSwitch(builder, codegen(ast->one), end_block, case_count);

	for (i = 0; i < case_count; i++)
		LLVMAddCase(switch_, case_vals[i], case_blocks[i]);

	LLVMPositionBuilderAtEnd(builder, end_block);

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
		LLVMAddDestination(branch, label_blocks[i]);

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	next_block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, next_block);

	return NULL;
}

LLVMValueRef gen_return(struct node *ast)
{
	LLVMValueRef func, retval;
	LLVMBasicBlockRef next_block, ret_block;

	ret_block = symtab_find(".return");
	retval = symtab_find(".retval");

	if (ast->one)
		LLVMBuildStore(builder, codegen(ast->one), retval);

	LLVMBuildBr(builder, ret_block);

	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	next_block = LLVMAppendBasicBlock(func, "");
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
	LLVMValueRef var, array;
	char *name;
	int size;

	name = ast->one->val;
	var = LLVMBuildAlloca(builder, TYPE_INT, name);

	if (ast->two) {
		size = LLVMConstIntGetZExtValue(codegen(ast->two));
		array = LLVMBuildAlloca(builder, TYPE_ARRAY(size), "");
		LLVMBuildStore(builder, lvalue_to_rvalue(array), var);
	}

	symtab_enter(name, var);

	return NULL;
}

static LLVMValueRef lvalue_name(struct node *ast)
{
	LLVMValueRef lvalue;

	lvalue = symtab_find(ast->val);

	if (lvalue == NULL)
		generror("un %s", ast->val);

	return lvalue;
}

static LLVMValueRef lvalue_indir(struct node *ast)
{
	return rvalue_to_lvalue(codegen(ast->one));
}

static LLVMValueRef lvalue_index(struct node *ast)
{
	LLVMValueRef ptr;

	ptr = LLVMBuildAdd(builder, codegen(ast->one), codegen(ast->two), "");

	return rvalue_to_lvalue(ptr);
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
		generror("lv");

	return NULL;
}

LLVMValueRef gen_assign(struct node *ast)
{

	LLVMValueRef result;

	result = codegen(ast->two);
	/* TODO: Forbid assignment to labels */
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_mul_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_mul(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_div_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_div(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_mod_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_mod(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_add_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_add(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_sub_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_sub(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_left_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_left(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_right_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_right(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_and_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_and(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_or_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_or(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_eq_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_eq(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_ne_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_ne(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_lt_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_lt(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_le_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_le(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_gt_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_gt(ast);
	check_store(result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_ge_assign(struct node *ast)
{
	LLVMValueRef result;

	result = gen_ge(ast);
	check_store(result, lvalue(ast->one));

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
			LLVMIntSLE,
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
	return lvalue_to_rvalue(lvalue(ast->one));
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

	check_store(result, lvalue(ast->one));

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

	check_store(result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_predec(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildSub(builder,
			codegen(ast->one),
			CONST(1),
			"");

	check_store(result, lvalue(ast->one));

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

	check_store(result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_index(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "");
}

LLVMValueRef gen_call(struct node *ast)
{
	LLVMValueRef func, *arg_list = NULL;
	struct node *n;
	int arg_count, i;

	func = LLVMBuildBitCast(builder,
			rvalue_to_lvalue(codegen(ast->one)),
			LLVMPointerType(TYPE_FUNC, 0),
			"");

	arg_count = count_chain(ast->two);
	arg_list = calloc(sizeof(LLVMValueRef), arg_count);

	if (arg_count > 0 && arg_list == NULL)
		generror("out of memory");

	for (i = 0, n = ast->two; i < arg_count; i++, n = n->two)
		arg_list[arg_count - i - 1] = codegen(n->one);

	return LLVMBuildCall(builder, func, arg_list, arg_count, "");
}

LLVMValueRef gen_name(struct node *ast)
{
	LLVMValueRef func, ptr, val;
	LLVMTypeRef type;

	ptr = lvalue(ast);
	type = LLVMTypeOf(ptr);

	if (LLVMGetTypeKind(type) == LLVMLabelTypeKind) {
		func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
		return LLVMBuildPtrToInt(builder,
				LLVMBlockAddress(func, (LLVMBasicBlockRef)ptr),
				TYPE_INT,
				"");
	}

	type = LLVMGetElementType(LLVMTypeOf(ptr));

	switch (LLVMGetTypeKind(type)) {
	case LLVMIntegerTypeKind:
		val = LLVMBuildLoad(builder, ptr, ast->val);

		if (LLVMIsAGlobalValue(ptr))
			val = LLVMBuildLShr(builder, val, CONST(WORDPOW), "");

		return val;

	default:
		generror("unexpected type '%s'", LLVMPrintTypeToString(type));
		return NULL;
	}
}

static char escape(char c)
{
	switch (c) {
	case '0':
		return '\0';
	case 'e':
		return EOT;
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
		return c;
	}
}

static long pack_char(const char **str)
{
	union {
		char buf[WORDSIZE];
		long intval;
	} pack = {{0}};

	const char *p;
	int n = 0;
	char c;

	p = *str;

	while (p[1] != '\0' && n < WORDSIZE) {
		c = p[0];

		if (c == '*') {
			c = escape(p[1]);
			p++;
		}

		pack.buf[n] = c;
		n++;
		p++;
	}

	if (p[1] == '\0')
		*str = NULL;
	else
		*str = p;

	return pack.intval;
}

static LLVMValueRef make_char(const char *str)
{
	LLVMValueRef charval;

	/* Skip leading ' */
	str += 1;
	charval = CONST(pack_char(&str));

	return charval;
}

static LLVMValueRef make_str(const char *str)
{
	LLVMValueRef global, strval, chars[MAX_STRSIZE];
	const char *p;
	int size = 0;

	/* Skip leading " */
	p = str + 1;
	while (p && size < MAX_STRSIZE - 1)
		chars[size++] = CONST(pack_char(&p));

	chars[size++] = CONST(EOT);

	global = LLVMAddGlobal(module, TYPE_ARRAY(size), ".gstr");
	LLVMSetLinkage(global, LLVMPrivateLinkage);

	strval = LLVMConstArray(TYPE_ARRAY(size), chars, size);
	LLVMSetInitializer(global, strval);

	return lvalue_to_rvalue(global);
}

LLVMValueRef gen_const(struct node *ast)
{
	switch (ast->val[0]) {
	case '\'':
		return make_char(ast->val);
	case '"':
		return make_str(ast->val);
	case '0':
		/* TODO: Support 09 and 08: see section 4.1.4 */
		return LLVMConstIntOfString(TYPE_INT, ast->val, 8);
	default:
		return LLVMConstIntOfString(TYPE_INT, ast->val, 10);
	}
}
