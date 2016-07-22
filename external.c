#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "cgram.tab.h"
#include "external.h"

static LLVMBuilderRef builder;
static LLVMModuleRef module;

LLVMValueRef codegen(struct node *ast)
{
	return ast->codegen(ast);
}

struct node *one(struct node *ast)
{
	return ast->one.ast;
}

struct node *two(struct node *ast)
{
	return ast->two.ast;
}

struct node *three(struct node *ast)
{
	return ast->three.ast;
}

char *leafval(struct node *ast)
{
	return ast->one.val;
}

void yyerror(const char *msg)
{
	printf("\n%*s\n%*s\n", lex_column, "^", lex_column, msg);
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

LLVMValueRef gen_compound(struct node *ast)
{
	return codegen(one(ast));
}

LLVMValueRef gen_index(struct node *ast)
{
	LLVMValueRef gep, indices[2];

	indices[0] = LLVMConstInt(LLVMInt64Type(), 0, 0);
	indices[1] = codegen(two(ast));

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

LLVMValueRef gen_vecdef(struct node *ast)
{
		/*
		 * TODO: Make a note that a "vector" in B terminology equates to
		 * an LLVM array, not an LLVM vector
		 */
		LLVMValueRef global, array, *elems;
		LLVMTypeRef type;
		struct node *p = three(ast);
		uint64_t i, size = 0, minsize = 0;

		while (p) {
			size++;
			p = p->one.ast;
		}

		if (two(ast))
			/*
			 * TODO: check that type is not string;
			 * use convenience function for handling
			 * chars and octal constants
			 * TODO: Check for invalid (negative) array size
			 */
			minsize = atoi(leafval(two(ast)));

		elems = calloc(sizeof(LLVMValueRef), size >= minsize ? size : minsize);
		/* TODO: Check all allocs for errors */
		if (!elems)
			generror("Out of memory");

		p = three(ast);
		i = size;
		while (p) {
			/* TODO: handle NAMES (convert global pointer to int) */
			elems[--i] = codegen(p->two.ast);
			p = p->one.ast;
		}

		i = size;
		while (i < minsize)
			elems[i++] = LLVMConstInt(LLVMInt64Type(), 0, 0);


		type = LLVMArrayType(LLVMInt64Type(), size);
		/* TODO: Figure out why "foo[6];" has size of 0 */
		array = LLVMConstArray(type, elems, size >= minsize ? size : minsize);

		global = LLVMAddGlobal(module, type, leafval(one(ast)));
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
	return LLVMBuildPtrToInt(builder, myvar, LLVMInt64Type(), "addr");
}

LLVMValueRef gen_indir(struct node *ast)
{
	/* TODO: Different semantics if being assigned to? */

	return LLVMBuildLoad(builder,
		LLVMBuildIntToPtr(
			builder,
			codegen(one(ast)),
			LLVMPointerType(LLVMInt64Type(), 0),
			"indir"),
		"loadtmp");

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
		"lttmp");
}

LLVMValueRef gen_add(struct node *ast)
{
	return LLVMBuildAdd(builder,
		codegen(one(ast)),
		codegen(two(ast)),
		"addtmp");
}

LLVMValueRef gen_auto(struct node *ast)
{
	/*
	* TODO: store name to indicate it was initialized.
	* also set up vector initialization.
	* TODO: Warn when using unitialized var
	* TODO: Determine type to use at runtime (or maybe always use Int64..
	* see "http://llvm.org/docs/GetElementPtr.html#how-is-gep-different-from-ptrtoint-arithmetic-and-inttoptr" -- LLVM assumes pointers are <= 64 bits
	* accept commandline argument or look at sizeof(void *)
	*/
	myvar = LLVMBuildAlloca(builder, LLVMInt64Type(), leafval(one(one(ast))));
	return myvar;
}

LLVMValueRef gen_name(struct node *ast)
{
	/* TODO: Retrieve variables by name */
	if (!myvar) {
		/* TODO: make global if not declared */
		generror("Wuh oh, attempted to access unitialized variable");
	}
	LLVMValueRef tmp = LLVMBuildLoad(builder, myvar, ast->one.val);
	return tmp;

}

LLVMValueRef gen_assign(struct node *ast)
{
	LLVMValueRef result = codegen(two(ast));
	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef gen_add_assign(struct node *ast)
{
	LLVMValueRef result;
	LLVMValueRef lhs = codegen(one(ast));
	LLVMValueRef rhs = codegen(two(ast));


	if (lhs == NULL || rhs == NULL) {
		return NULL;
	}

	result = LLVMBuildAdd(builder, lhs, rhs, "addtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef gen_postdec(struct node *ast)
{
	LLVMValueRef result;
	result = LLVMBuildSub(builder,
		codegen(one(ast)),
		LLVMConstInt(LLVMInt64Type(), 1, 0),
		"subtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef gen_predec(struct node *ast)
{
	LLVMValueRef tmp = myvar;
	myvar = LLVMBuildSub(builder,
		codegen(one(ast)),
		LLVMConstInt(LLVMInt64Type(), -1, 0),
		"subtmp");

	return tmp;
}

LLVMValueRef gen_const(struct node *ast)
{
	/* TODO: Handle strings, multichars, escape sequences, and octal ints */
	if (ast->one.val[0] == '"')
		return NULL;
	else if (ast->one.val[0] == '\'')
		return LLVMConstInt(LLVMInt64Type(), ast->one.val[1], 0);
	else
		return LLVMConstIntOfString(LLVMInt64Type(), ast->one.val, 10);
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
	LLVMTypeRef intarg = LLVMInt64Type();

	signew = LLVMFunctionType(LLVMInt64Type(), &intarg, 1, 0);
	/* TODO: Macro for accessing leaf val: LEAFVAL(ast->one) */
	if (first) {
		funcnew = LLVMAddGlobal(module, signew, leafval(one(ast)));
		LLVMSetLinkage(funcnew, LLVMExternalLinkage);
		first = 0;
	} else {
		funcnew = LLVMGetNamedGlobal(module, leafval(one(ast)));
	}
	//LLVMInsertIntoBuilder(builder, funcnew);
	//func = LLVMGetNamedGlobal(module, leafval(one(ast)));
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
	sig = LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0);
	func = LLVMAddFunction(module, leafval(one(ast)), sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	block = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, block);

	retval = LLVMBuildAlloca(builder, LLVMInt64Type(), "retval");
	LLVMBuildStore(builder,
		LLVMConstInt(LLVMInt64Type(), 0, 0),
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

/* TODO: rename struct node to ast_node, rename arg ast to node */
void compile(struct node *ast)
{
	/* TODO: Free module, define "dbc" as constant */
	/* TODO: Make builder/module static in codegen so we don't have to pass them around */

	builder = LLVMCreateBuilder();
	module = LLVMModuleCreateWithName("dbc");


	/* TODO: Remove superfluous returns from gen_ */
	codegen(ast);
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
