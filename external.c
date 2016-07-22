#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "types.h"
#include "external.h"

static LLVMBuilderRef builder;
static LLVMModuleRef module;

void yyerror(const char *msg)
{
	printf("\n%*s\n%*s\n", column, "^", column, msg);
	printf("column %d\n", column);
	fflush(stdout);
}

void generror(const char *msg)
{
	printf("ERROR: %s\n", msg);
	fflush(stdout);
	exit(EXIT_FAILURE);
}


static LLVMValueRef myvar = NULL;
static LLVMBasicBlockRef mylabel = NULL;

LLVMValueRef codegen_compound(struct node *ast)
{
	return ast->one.ast->codegen(ast->one.ast);
}

LLVMValueRef codegen_index(struct node *ast)
{
	LLVMValueRef gep, indices[2];

	indices[0] = LLVMConstInt(LLVMInt64Type(), 0, 0);
	indices[1] = ast->two.ast->codegen(ast->two.ast);

	/*
	 * TODO: allow indexing arbitrary expressions
	 * TODO: ensure x[y] == y[x] holds
	 */
	gep = LLVMBuildGEP(builder,
		LLVMGetNamedGlobal(module, "myarray"),
		indices,
		2,
		"tmp_subscript");

	return LLVMBuildLoad(builder, gep, "tmp_load");
}

LLVMValueRef codegen_vecdef(struct node *ast)
{
		/*
		 * TODO: Make a note that a "vector" in B terminology equates to
		 * an LLVM array, not an LLVM vector
		 */
		LLVMValueRef global, array, *elems;
		LLVMTypeRef type;
		struct node *p = ast->three.ast;
		uint64_t i, size = 0, minsize = 0;

		while (p) {
			size++;
			p = p->one.ast;
		}

		if (ast->two.ast)
			/*
			 * TODO: check that type is not string;
			 * use convenience function for handling
			 * chars and octal constants
			 * TODO: Check for invalid (negative) array size
			 */
			minsize = atoi(ast->two.ast->one.val);

		elems = calloc(sizeof(LLVMValueRef), size >= minsize ? size : minsize);
		/* TODO: Check all allocs for errors */
		if (!elems)
			generror("Out of memory");

		p = ast->three.ast;
		i = size;
		while (p) {
			/* TODO: handle NAMES (convert global pointer to int) */
			elems[--i] = p->two.ast->codegen(p->two.ast);
			p = p->one.ast;
		}

		i = size;
		while (i < minsize)
			elems[i++] = LLVMConstInt(LLVMInt64Type(), 0, 0);


		type = LLVMArrayType(LLVMInt64Type(), size);
		/* TODO: Figure out why "foo[6];" has size of 0 */
		array = LLVMConstArray(type, elems, size >= minsize ? size : minsize);

		global = LLVMAddGlobal(module, type, ast->one.ast->one.val);
		LLVMSetInitializer(global, array);
		LLVMSetLinkage(global, LLVMExternalLinkage);

		return global;
}

LLVMValueRef codegen_expression(struct node *ast)
{
	return NULL;
}

LLVMValueRef retval;
LLVMValueRef codegen_return(struct node *ast)
{
	if (ast->one.ast)
		LLVMBuildStore(
			builder,
			ast->one.ast->codegen(ast->one.ast),
			retval);

	return NULL;
}

LLVMValueRef codegen_label(struct node *ast)
{
	LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	mylabel = LLVMAppendBasicBlock(parent, "label");
	return ast->one.ast->codegen(ast->one.ast);
}

LLVMValueRef codegen_goto(struct node *ast)
{
	return LLVMBuildBr(builder, mylabel);
}

LLVMValueRef codegen_addr(struct node *ast)
{
	/* TODO: Function pointers? */
	return LLVMBuildPtrToInt(builder, myvar, LLVMInt64Type(), "addr");
}

LLVMValueRef codegen_indir(struct node *ast)
{
	/* TODO: Different semantics if being assigned to? */

	return LLVMBuildLoad(builder,
		LLVMBuildIntToPtr(
			builder,
			ast->one.ast->codegen(ast->one.ast),
			LLVMPointerType(LLVMInt64Type(), 0),
			"indir"),
		"loadtmp");

}

LLVMValueRef codegen_while(struct node *ast)
{
	LLVMValueRef condition, zero, func, body_value;
	LLVMBasicBlockRef body_block, end;

	condition = ast->one.ast->codegen(ast->one.ast);
	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "ifcond");
	/* TODO: func isn't always a func... rename to "block" */
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	body_block = LLVMAppendBasicBlock(func, "body");
	end = LLVMAppendBasicBlock(func, "end");

	LLVMBuildCondBr(builder, condition, body_block, end);
	LLVMPositionBuilderAtEnd(builder, body_block);
	/* TODO: I don't think we need to collect values from then/else blocks */
	body_value = ast->two.ast->codegen(ast->two.ast);

	LLVMBuildBr(builder, end);
	/* TODO: What's the point of this? */
	body_block = LLVMGetInsertBlock(builder);

	LLVMPositionBuilderAtEnd(builder, end);

	return body_value;
}

LLVMValueRef codegen_if(struct node *ast)
{
	LLVMValueRef zero, condition, parent, ref;
	LLVMBasicBlockRef then_block, else_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = ast->one.ast->codegen(ast->one.ast);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "tmp_cond");
	parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(parent, "then");
	else_block = LLVMAppendBasicBlock(parent, "else");
	end = LLVMAppendBasicBlock(parent, "end");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	ast->two.ast->codegen(ast->two.ast);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	ast->three.ast->codegen(ast->three.ast);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, end);
	return ref;
}

LLVMValueRef codegen_lt(struct node *ast)
{
	return LLVMBuildICmp(builder,
		LLVMIntSLT,
		ast->one.ast->codegen(ast->one.ast),
		ast->two.ast->codegen(ast->two.ast),
		"lttmp");
}

LLVMValueRef codegen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
		ast->one.ast->codegen(ast->one.ast),
		ast->two.ast->codegen(ast->two.ast),
		"addtmp");
}

LLVMValueRef codegen_auto(struct node *ast)
{
	/*
	* TODO: store name to indicate it was initialized.
	* also set up vector initialization.
	* TODO: Warn when using unitialized var
	* TODO: Determine type to use at runtime (or maybe always use Int64..
	* see "http://llvm.org/docs/GetElementPtr.html#how-is-gep-different-from-ptrtoint-arithmetic-and-inttoptr" -- LLVM assumes pointers are <= 64 bits
	* accept commandline argument or look at sizeof(void *)
	*/
	printf("Alloca'ing %s\n", ast->one.ast->one.ast->one.val);
	myvar = LLVMBuildAlloca(builder, LLVMInt64Type(), ast->one.ast->one.ast->one.val);
	return myvar;
}

LLVMValueRef codegen_name(struct node *ast)
{
	/* TODO: Retrieve variables by name */
	if (!myvar) {
		/* TODO: make global if not declared */
		generror("Wuh oh, attempted to access unitialized variable");
	}
	printf("Building loader for %s\n", ast->one.val);
	printf("@@@@@@@@\n");
	LLVMDumpValue(myvar);
	printf("@@@@@@@@\n");
	LLVMValueRef tmp = LLVMBuildLoad(builder, myvar, ast->one.val);
	printf("########\n");
	return tmp;

}

LLVMValueRef codegen_assign(struct node *ast)
{
	LLVMValueRef result = ast->two.ast->codegen(ast->two.ast);
	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_add_assign(struct node *ast)
{
	LLVMValueRef result;
	LLVMValueRef lhs = ast->one.ast->codegen(ast->one.ast);
	LLVMValueRef rhs = ast->two.ast->codegen(ast->two.ast);


	if (lhs == NULL || rhs == NULL) {
		return NULL;
	}

	result = LLVMBuildAdd(builder, lhs, rhs, "addtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_postdec(struct node *ast)
{
	LLVMValueRef result;
	result = LLVMBuildSub(builder,
		ast->one.ast->codegen(ast->one.ast),
		LLVMConstInt(LLVMInt64Type(), 1, 0),
		"subtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_predec(struct node *ast)
{
	LLVMValueRef tmp = myvar;
	myvar = LLVMBuildSub(builder,
		ast->one.ast->codegen(ast->one.ast),
		LLVMConstInt(LLVMInt64Type(), -1, 0),
		"subtmp");

	return tmp;
}

LLVMValueRef codegen_const(struct node *ast)
{
	/* TODO: Handle strings, multichars, escape sequences, and octal ints */
	if (ast->one.val[0] == '"')
		return NULL;
	else if (ast->one.val[0] == '\'')
		return LLVMConstInt(LLVMInt64Type(), ast->one.val[1], 0);
	else
		return LLVMConstIntOfString(LLVMInt64Type(), ast->one.val, 10);
}


LLVMValueRef codegen_statements(struct node *ast)
{
	ast->one.ast->codegen(ast->one.ast);
	return ast->two.ast->codegen(ast->two.ast);
}


LLVMValueRef codegen_call(struct node *ast)
{
	static int first = 1;
	LLVMValueRef func, *args;

	LLVMTypeRef signew;
	LLVMValueRef funcnew;
	LLVMTypeRef intarg = LLVMInt64Type();

	signew = LLVMFunctionType(LLVMInt64Type(), &intarg, 1, 0);
	/* TODO: Macro for accessing leaf val: LEAFVAL(ast->one) */
	if (first) {
		funcnew = LLVMAddGlobal(module, signew, ast->one.ast->one.val);
		LLVMSetLinkage(funcnew, LLVMExternalLinkage);
		first = 0;
	} else {
		funcnew = LLVMGetNamedGlobal(module, ast->one.ast->one.val);
	}
	//LLVMInsertIntoBuilder(builder, funcnew);
	//func = LLVMGetNamedGlobal(module, ast->one.ast->one.val);
	func = funcnew;

	if (func == NULL) {
		return NULL;
	}

	args = malloc(sizeof(LLVMValueRef));
	/* TODO: support multiple args */
	*args = ast->two.ast->codegen(ast->two.ast);

	return LLVMBuildCall(builder, func, args, 1, "calltmp");
}

LLVMValueRef codegen_extrn(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMValueRef func;

	sig = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
	func = LLVMAddGlobal(module, sig, "putchar");
	LLVMSetLinkage(func, LLVMExternalLinkage);
	return func;
}

LLVMValueRef codegen_funcdef(struct node *ast)
{
	LLVMTypeRef sig;
	LLVMValueRef func, body;
	LLVMBasicBlockRef block;

	/* TODO: Check if function already defined */
	sig = LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0);
	func = LLVMAddFunction(module, ast->one.ast->one.val, sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, block);

	retval = LLVMBuildAlloca(builder, LLVMInt64Type(), "retval");
	LLVMBuildStore(builder,
		LLVMConstInt(LLVMInt64Type(), 0, 0),
		retval);
	body = ast->three.ast->codegen(ast->three.ast);

	LLVMBuildRet(builder, LLVMBuildLoad(builder, retval, "retval"));

	if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
		LLVMDeleteFunction(func);
		return NULL;
	}

	return func;
}

LLVMValueRef codegen_defs(struct node *ast)
{

	ast->one.ast->codegen(ast->one.ast);
	return ast->two.ast->codegen(ast->two.ast);
}

LLVMValueRef codegen_and(struct node *ast) { generror("Not yet implemented: codegen_and"); return NULL; }
LLVMValueRef codegen_and_assign(struct node *ast) { generror("Not yet implemented: codegen_and_assign"); return NULL; }
LLVMValueRef codegen_args(struct node *ast) { generror("Not yet implemented: codegen_args"); return NULL; }
LLVMValueRef codegen_case(struct node *ast) { generror("Not yet implemented: codegen_case"); return NULL; }
LLVMValueRef codegen_comma(struct node *ast) { generror("Not yet implemented: codegen_comma"); return NULL; }
LLVMValueRef codegen_cond(struct node *ast) { generror("Not yet implemented: codegen_cond"); return NULL; }
LLVMValueRef codegen_div(struct node *ast) { generror("Not yet implemented: codegen_div"); return NULL; }
LLVMValueRef codegen_div_assign(struct node *ast) { generror("Not yet implemented: codegen_div_assign"); return NULL; }
LLVMValueRef codegen_eq(struct node *ast) { generror("Not yet implemented: codegen_eq"); return NULL; }
LLVMValueRef codegen_eq_assign(struct node *ast) { generror("Not yet implemented: codegen_eq_assign"); return NULL; }
LLVMValueRef codegen_ge(struct node *ast) { generror("Not yet implemented: codegen_ge"); return NULL; }
LLVMValueRef codegen_gt(struct node *ast) { generror("Not yet implemented: codegen_gt"); return NULL; }
LLVMValueRef codegen_init(struct node *ast) { generror("Not yet implemented: codegen_init"); return NULL; }
LLVMValueRef codegen_inits(struct node *ast) { generror("Not yet implemented: codegen_inits"); return NULL; }
LLVMValueRef codegen_ior(struct node *ast) { generror("Not yet implemented: codegen_ior"); return NULL; }
LLVMValueRef codegen_ivals(struct node *ast) { generror("Not yet implemented: codegen_ivals"); return NULL; }
LLVMValueRef codegen_le(struct node *ast) { generror("Not yet implemented: codegen_le"); return NULL; }
LLVMValueRef codegen_left(struct node *ast) { generror("Not yet implemented: codegen_left"); return NULL; }
LLVMValueRef codegen_left_assign(struct node *ast) { generror("Not yet implemented: codegen_left_assign"); return NULL; }
LLVMValueRef codegen_mod(struct node *ast) { generror("Not yet implemented: codegen_mod"); return NULL; }
LLVMValueRef codegen_mod_assign(struct node *ast) { generror("Not yet implemented: codegen_mod_assign"); return NULL; }
LLVMValueRef codegen_mul(struct node *ast) { generror("Not yet implemented: codegen_mul"); return NULL; }
LLVMValueRef codegen_mul_assign(struct node *ast) { generror("Not yet implemented: codegen_mul_assign"); return NULL; }
LLVMValueRef codegen_names(struct node *ast) { generror("Not yet implemented: codegen_names"); return NULL; }
LLVMValueRef codegen_ne(struct node *ast) { generror("Not yet implemented: codegen_ne"); return NULL; }
LLVMValueRef codegen_ne_assign(struct node *ast) { generror("Not yet implemented: codegen_ne_assign"); return NULL; }
LLVMValueRef codegen_neg(struct node *ast) { generror("Not yet implemented: codegen_neg"); return NULL; }
LLVMValueRef codegen_not(struct node *ast) { generror("Not yet implemented: codegen_not"); return NULL; }
LLVMValueRef codegen_or_assign(struct node *ast) { generror("Not yet implemented: codegen_or_assign"); return NULL; }
LLVMValueRef codegen_postinc(struct node *ast) { generror("Not yet implemented: codegen_postinc"); return NULL; }
LLVMValueRef codegen_preinc(struct node *ast) { generror("Not yet implemented: codegen_preinc"); return NULL; }
LLVMValueRef codegen_right(struct node *ast) { generror("Not yet implemented: codegen_right"); return NULL; }
LLVMValueRef codegen_right_assign(struct node *ast) { generror("Not yet implemented: codegen_right_assign"); return NULL; }
LLVMValueRef codegen_simpledef(struct node *ast) { generror("Not yet implemented: codegen_simpledef"); return NULL; }
LLVMValueRef codegen_sub(struct node *ast) { generror("Not yet implemented: codegen_sub"); return NULL; }
LLVMValueRef codegen_sub_assign(struct node *ast) { generror("Not yet implemented: codegen_sub_assign"); return NULL; }
LLVMValueRef codegen_switch(struct node *ast) { generror("Not yet implemented: codegen_switch"); return NULL; }
LLVMValueRef codegen_xor(struct node *ast) { generror("Not yet implemented: codegen_xor"); return NULL; }
LLVMValueRef codegen_xor_assign(struct node *ast) { generror("Not yet implemented: codegen_xor_assign"); return NULL; }

/* TODO: rename struct node to ast_node, rename arg ast to node */
void compile(struct node *ast)
{
	/* TODO: Free module, define "dbc" as constant */
	/* TODO: Make builder/module static in codegen so we don't have to pass them around */

	builder = LLVMCreateBuilder();
	module = LLVMModuleCreateWithName("dbc");


	/* TODO: Remove superfluous returns from codegen_ */
	ast->codegen(ast);
	printf("\n====================================\n");
	LLVMDumpModule(module);
	printf("====================================\n");

	if (LLVMWriteBitcodeToFile(module, "cgram.bc") != 0) {
		generror("Failed to write bitcode");
	}
}

void free_tree(struct node *ast)
{
	/* TODO:  Free while compiling instead? */
}

struct node *leaf(LLVMValueRef (*codegen)(struct node *), char *value)
{
	struct node *nleaf = malloc(sizeof(struct node));
	nleaf->codegen = codegen;
	nleaf->one.val = value;
	nleaf->two.ast = NULL;
	nleaf->three.ast = NULL;

	return nleaf;
}

struct node *node3(LLVMValueRef (*codegen)(struct node *), struct node *one, struct node *two, struct node *three)
{
	/* TODO: Error handling */
	struct node *nnode = malloc(sizeof(struct node));

	nnode->codegen = codegen;
	nnode->one.ast = one;
	nnode->two.ast = two;
	nnode->three.ast = three;

	return nnode;
}

struct node *node2(LLVMValueRef (*codegen)(struct node *), struct node *one, struct node *two)
{
	return node3(codegen, one, two, NULL);
}

struct node *node1(LLVMValueRef (*codegen)(struct node *), struct node *one)
{
	return node3(codegen, one, NULL, NULL);
}

struct node *node0(LLVMValueRef (*codegen)(struct node *))
{
	return node3(codegen, NULL, NULL, NULL);
}
