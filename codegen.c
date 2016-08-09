#include <limits.h>
#include <search.h>
#include <stdarg.h>
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

static LLVMBuilderRef builder;
static LLVMModuleRef module;


/* TODO: Take all helper functions out of header and make static */
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

/* TODO: rename struct node to ast_node, rename arg ast to node */
void compile(struct node *ast)
{
	/* TODO: Free module, define "dbc" as constant */
	if ((module = LLVMModuleCreateWithName("dbc")) == NULL)
		generror("Failed to create LLVM module");

	if ((builder = LLVMCreateBuilder()) == NULL)
		generror("Failed to create LLVM instruction builder");

	/* TODO: Remove superfluous returns from gen_ */
	/* TODO: Verify module (LLVMVerifyModule) */
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
static LLVMValueRef func;
static LLVMValueRef retval;
static LLVMBasicBlockRef ret_block;
static LLVMBasicBlockRef labels[MAX_LABELS];
static LLVMBasicBlockRef cases[MAX_SWITCHES][MAX_CASES];
static LLVMValueRef switches[MAX_SWITCHES];
static int case_count[MAX_SWITCHES];
static int label_count, switch_count, case_i;

LLVMValueRef gen_compound(struct node *ast)
{
	if (ast->one)
		return codegen(ast->one);

	return NULL;
}

LLVMValueRef gen_index(struct node *ast)
{
	return LLVMBuildLoad(builder, lvalue(ast), "tmp_load");
}

static int count_chain(struct node *ast)
{
	if (ast == NULL)
		return 0;

	return 1 + count_chain(ast->two);
}

LLVMValueRef gen_ivals(struct node *ast)
{
	LLVMValueRef *ival_list;
	int size, i;

	size = count_chain(ast);
	ival_list = calloc(sizeof(LLVMValueRef), size);

	if (ival_list == NULL)
		generror("out of memory");

	for (i = 0; i < size; i++, ast = ast->two)
		ival_list[i] = codegen(ast->one);

	return LLVMConstArray(TYPE_ARRAY(size), ival_list, size);
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

	/* TODO: Check all allocs for errors */
	ival_list = calloc(sizeof(LLVMValueRef), size);
	if (ival_list == NULL)
		generror("out of memory");

	for (i = 0, n = ast->three; i < initsize; i++, n = n->two)
		/* TODO: handle NAMES (convert global pointer to int) */
		ival_list[i] = codegen(n->one);

	for (i = initsize; i < size; i++)
		ival_list[i] = LLVMConstInt(TYPE_INT, 0, 0);

	global = LLVMGetNamedGlobal(module, ast->one->val);
	init = LLVMConstArray(TYPE_ARRAY(size), ival_list, size);
	LLVMSetInitializer(global, init);

	return NULL;
}

LLVMValueRef gen_null(struct node *ast)
{
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

LLVMValueRef gen_goto(struct node *ast)
{
	LLVMValueRef branch;
	LLVMBasicBlockRef next_block;
	int i;

	branch = LLVMBuildIndirectBr(builder,
		LLVMBuildIntToPtr(
			builder,
			codegen(ast->one),
			TYPE_LABEL,
			"tmp_label"),
		label_count);

	for (i = 0; i < label_count; i++)
		LLVMAddDestination(branch, labels[i]);


	next_block = LLVMAppendBasicBlock(func, "after_goto");
	LLVMPositionBuilderAtEnd(builder, next_block);

	return NULL;
}

LLVMValueRef gen_addr(struct node *ast)
{
	/* TODO: Function pointers? */
	/* TODO: Check that lvalue is actually an lvalue */
	return LLVMBuildPtrToInt(builder, lvalue(ast->one), TYPE_INT, "tmp_addr");
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
	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(ast->one), zero, "tmp_cond");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	do_block = LLVMAppendBasicBlock(func, "do");
	end = LLVMAppendBasicBlock(func, "end");

	LLVMBuildCondBr(builder, condition, do_block, end);
	LLVMPositionBuilderAtEnd(builder, do_block);
	/* TODO: I don't think we need to collect values from then/else blocks */
	body_value = codegen(ast->two);

	condition = LLVMBuildICmp(builder, LLVMIntNE, codegen(ast->one), zero, "tmp_cond");
	LLVMBuildCondBr(builder, condition, do_block, end);

	LLVMPositionBuilderAtEnd(builder, end);

	return body_value;
}

LLVMValueRef gen_if(struct node *ast)
{
	LLVMValueRef zero, condition, func, ref;
	LLVMBasicBlockRef then_block, else_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = codegen(ast->one);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "tmp_cond");
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(func, "then");
	else_block = LLVMAppendBasicBlock(func, "else");
	end = LLVMAppendBasicBlock(func, "end");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	codegen(ast->two);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	if (ast->three)
		codegen(ast->three);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, end);
	return ref;
}

LLVMValueRef gen_lt(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSLT,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_lt");
}

LLVMValueRef gen_cond(struct node *ast)
{
	/* TODO: Rename to avoid conflict with IF condition */
	/* TODO: Figure out best way to convert to and from Int1 */
	return LLVMBuildSelect(builder,
		LLVMBuildICmp(builder,
			LLVMIntNE,
			codegen(ast->one),
			LLVMConstInt(TYPE_INT, 0, 0),
			"tmp_not"),
		codegen(ast->two),
		codegen(ast->three),
		"tmp_cond");
}

LLVMValueRef gen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_add");
}

LLVMValueRef gen_sub(struct node *ast)
{
	/* TODO: Prefix names with "." so they don't collide with variable names? */
	return LLVMBuildSub(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_sub");
}

LLVMValueRef gen_mul(struct node *ast)
{
	return LLVMBuildMul(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_mul");
}

LLVMValueRef gen_mod(struct node *ast)
{
	return LLVMBuildSRem(builder,
		codegen(ast->one),
		codegen(ast->two),
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

	LLVMValueRef var;
	LLVMTypeRef type;
	struct node *init_list, *init;
	char *name;

	init_list = ast->one;

	while (init_list) {
		init = init_list->one;
		name = init->one->val;

		/* TODO: replaces calls to atol with function that gets
		 * value of any constant literal */
		type = init->two ? TYPE_ARRAY(atol(init->two->val)) : TYPE_INT;
		var = LLVMBuildAlloca(builder, type, name);

		symtab_enter(name, var);

		init_list = init_list->two;
	}

	codegen(ast->two);

	return NULL;
}

LLVMValueRef gen_name(struct node *ast)
{
	LLVMValueRef ptr;
	LLVMTypeRef type;

	ptr = lvalue(ast);

	/* TODO: Is there a nicer way of doing this without special casing labels? */
	/* TODO: Convert function pointers to int */
	if (LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMLabelTypeKind)
		return LLVMBuildPtrToInt(
			builder,
			LLVMBlockAddress(func, (LLVMBasicBlockRef)ptr),
			TYPE_INT,
			"tmp_addr");

	type = LLVMGetElementType(LLVMTypeOf(ptr));

	switch (LLVMGetTypeKind(type)) {
		case LLVMIntegerTypeKind:
			return LLVMBuildLoad(builder, ptr, ast->val);
		case LLVMArrayTypeKind:
		case LLVMFunctionTypeKind:
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
	LLVMValueRef lvalue;

	lvalue = symtab_find(ast->val);

	/* TODO: Allow undeclared functions for calls */
	if (lvalue == NULL)
		generror("Use of undeclared identifier '%s'", ast->val);

	return lvalue;
}

LLVMValueRef lvalue_indir(struct node *ast)
{
	return LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, "tmp_indir");
}

LLVMValueRef lvalue_index(struct node *ast)
{
	LLVMValueRef ptr, index, gep;

	/*
	 * TODO: ensure x[y] == y[x] holds
	 */
	ptr = LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, "tmp_ptr");
	index = codegen(ast->two);
	gep = LLVMBuildGEP(builder, ptr, &index, 1, "tmp_subscript");

	return gep;
}

LLVMValueRef gen_assign(struct node *ast)
{

	LLVMValueRef result;
	result = codegen(ast->two);
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_div_assign(struct node *ast)
{
	LLVMValueRef result, left, right;

	left = codegen(ast->one);
	right = codegen(ast->two);

	if (!left || !right)
		return NULL;

	result = LLVMBuildSDiv(builder, left, right, "tmp_div");
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_add_assign(struct node *ast)
{
	LLVMValueRef result, left, right;

	left = codegen(ast->one);
	right = codegen(ast->two);

	/* TODO: Can left or right ever be null? */
	if (!left || !right)
		return NULL;

	result = LLVMBuildAdd(builder, left, right, "tmp_add");
	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_postinc(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(ast->one);

	/* TODO: Add macro for creating constants */
	result = LLVMBuildAdd(builder,
		orig,
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_preinc(struct node *ast)
{
	LLVMValueRef result;

	/* TODO: Add macro for creating constants */
	result = LLVMBuildAdd(builder,
		codegen(ast->one),
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
}

LLVMValueRef gen_postdec(struct node *ast)
{
	LLVMValueRef orig, result;

	orig = codegen(ast->one);

	/* TODO: Add macro for creating constants */
	result = LLVMBuildSub(builder,
		orig,
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return orig;
}

LLVMValueRef gen_predec(struct node *ast)
{
	LLVMValueRef result;

	/* TODO: Add macro for creating constants */
	result = LLVMBuildSub(builder,
		codegen(ast->one),
		LLVMConstInt(TYPE_INT, 1, 0),
		"tmp_add");

	LLVMBuildStore(builder, result, lvalue(ast->one));

	return result;
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

	p = *str + 1;

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
		intval |= buf[i] << CHAR_BIT*(size - i - 1);

	return intval;
}

static LLVMValueRef make_char(const char *str)
{
	LLVMValueRef charval;

	charval = LLVMConstInt(TYPE_INT, pack_char(&str), 0);

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

	while (str && size < MAX_STRSIZE)
		chars[size++] = LLVMConstInt(TYPE_INT, pack_char(&str), 0);

	if (str)
		generror("warning: string constant too long");

	/* Terminate string with an EOF char left-aligned in a word */
	chars[size++] = LLVMConstInt(TYPE_INT, EOF << CHAR_BIT*(MAX_CHARSIZE - 1), 0);

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


LLVMValueRef gen_statements(struct node *ast)
{
	codegen(ast->one);
	return codegen(ast->two);
}


LLVMValueRef gen_call(struct node *ast)
{
	/* TODO: Check that existing global is a function with same # of args */
	/* TODO: support multiple args */
	/* TODO: Standardize variable naming between gen_call and gen_*def */
	LLVMValueRef func, *args;
	struct node *arg_list;
	int i = 0, arg_cnt = 0;

	arg_list = ast->two;
	while (arg_list) {
		arg_cnt++;
		arg_list = arg_list->two;
	}

	args = calloc(sizeof(LLVMValueRef), arg_cnt);

	if (args == NULL)
		generror("Out of memory");

	i = arg_cnt;
	arg_list = ast->two;
	while (arg_list) {
		args[--i] = codegen(arg_list->one);
		arg_list = arg_list->two;
	}

	func = LLVMBuildBitCast(builder,
		LLVMBuildIntToPtr(builder, codegen(ast->one), TYPE_PTR, "tmp_ptr"),
		LLVMPointerType(TYPE_FUNC, 0),
		"tmp_func");

	return LLVMBuildCall(builder, func, args, arg_cnt, "tmp_call");
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

static void predeclare_switches(struct node *ast)
{
	if (ast->one)
		predeclare_switches(ast->one);

	if (ast->two)
		predeclare_switches(ast->two);

	if (ast->three)
		predeclare_switches(ast->three);

	if (ast->codegen == gen_case) {
		/*
		if (switch_count == 0)
			generror("Warning: 'case' statement not in switch statement");
		*/

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

LLVMValueRef gen_switch(struct node *ast)
{
	LLVMBasicBlockRef next_block;

	next_block = LLVMAppendBasicBlock(func, "switch_end");

	switches[switch_count] = LLVMBuildSwitch(builder, codegen(ast->one), next_block, case_count[switch_count]);
	case_i = 0;
	codegen(ast->two);

	LLVMPositionBuilderAtEnd(builder, cases[switch_count][case_count[switch_count]-1]);
	LLVMBuildBr(builder, next_block);
	LLVMPositionBuilderAtEnd(builder, next_block);
	switch_count++;

	return NULL;
}

static void predeclare_labels(struct node *ast)
{
	LLVMBasicBlockRef label_block;
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
		label_block = LLVMAppendBasicBlock(func, name);
		//symtab_enter(name, LLVMBlockAddress(func, label_block));
		symtab_enter(name, label_block);

		if (label_count >= MAX_LABELS)
			generror("Label table overflow");

		labels[label_count++] = label_block;
	}
}

LLVMValueRef gen_funcdef(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMBasicBlockRef body_block;

	if (hcreate(SYMTAB_SIZE) == 0)
		generror("Failed to allocate space for symbol table");

	/* TODO: Check if function already defined
	 * ? Can functions be looked up like globals ? */
	sig = LLVMFunctionType(TYPE_INT, NULL, 0, 0);
	func = LLVMGetNamedFunction(module, ast->one->val);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	body_block = LLVMAppendBasicBlock(func, "");
	ret_block = LLVMAppendBasicBlock(func, "ret_block");
	LLVMPositionBuilderAtEnd(builder, body_block);

	retval = LLVMBuildAlloca(builder, TYPE_INT, "tmp_ret");
	LLVMBuildStore(builder, LLVMConstInt(TYPE_INT, 0, 0), retval);

	label_count = 0;
	predeclare_labels(ast->three);

	switch_count = 0;
	predeclare_switches(ast->three);
	switch_count = 0;

	codegen(ast->three);
	LLVMBuildBr(builder, ret_block);

	/* TODO: Untangle out-of-order blocks */
	LLVMPositionBuilderAtEnd(builder, ret_block);
	LLVMBuildRet(builder, LLVMBuildLoad(builder, retval, "tmp_ret"));

	LLVMMoveBasicBlockAfter(ret_block, LLVMGetLastBasicBlock(func));

	/* TODO: Handle failed verification and print internal compiler error */
	LLVMVerifyFunction(func, LLVMPrintMessageAction);

	hdestroy();

	return NULL;
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

static void predeclare_funcdef(struct node *ast)
{
	LLVMValueRef func;
	LLVMTypeRef sig;

	sig = LLVMFunctionType(TYPE_INT, NULL, 0, 0);
	func = LLVMAddFunction(module, ast->one->val, sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);
}

static void predeclare_defs(struct node *ast)
{
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

LLVMValueRef gen_not(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntEQ,
		codegen(ast->one),
		LLVMConstInt(TYPE_INT, 0, 0),
		"tmp_not");
}

LLVMValueRef gen_ne(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntNE,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_ne");
}

LLVMValueRef gen_eq(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntEQ,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_eq");
}

LLVMValueRef gen_and(struct node *ast)
{
	return LLVMBuildAnd(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_and");
}

LLVMValueRef gen_and_assign(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildAnd(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_and");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_div(struct node *ast)
{
	return LLVMBuildSDiv(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_div");
}

LLVMValueRef gen_eq_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildICmp(builder,
		LLVMIntEQ,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_eq");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_ge(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSGE,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_ge");
}

LLVMValueRef gen_gt(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSGT,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_gt");
}

LLVMValueRef gen_or(struct node *ast)
{
	return LLVMBuildOr(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_or");
}

LLVMValueRef gen_le(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSLT,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_gt");
}

LLVMValueRef gen_left(struct node *ast)
{
	return LLVMBuildShl(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_shl");
}

LLVMValueRef gen_left_assign(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildShl(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_shl");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_mod_assign(struct node *ast)
{
	LLVMValueRef result;

	result = LLVMBuildSRem(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_mod");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_mul_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildMul(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_mul");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_ne_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildICmp(builder,
		LLVMIntNE,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_ne");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_neg(struct node *ast)
{
	return LLVMBuildNeg(builder,
		codegen(ast->one),
		"tmp_neg");
}

LLVMValueRef gen_or_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildOr(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_or");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_right(struct node *ast)
{
	return LLVMBuildLShr(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_shr");
}

LLVMValueRef gen_right_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildLShr(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_shr");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}

LLVMValueRef gen_sub_assign(struct node *ast)
{
	LLVMValueRef result;

	result =  LLVMBuildSub(builder,
		codegen(ast->one),
		codegen(ast->two),
		"tmp_sub");

	LLVMBuildStore(builder, result, lvalue(ast->one));
	return result;
}


LLVMValueRef gen_args(struct node *ast) { generror("Not yet implemented: gen_args"); return NULL; }
LLVMValueRef gen_init(struct node *ast) { generror("Not yet implemented: gen_init"); return NULL; }
LLVMValueRef gen_inits(struct node *ast) { generror("Not yet implemented: gen_inits"); return NULL; }
LLVMValueRef gen_names(struct node *ast) { generror("Not yet implemented: gen_names"); return NULL; }
