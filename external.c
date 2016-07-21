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

void yyerror(const char *msg)
{
	printf("\n%*s\n%*s\n", column, "^", column, msg);
	fflush(stdout);
}

void generror(const char *msg)
{
	printf("ERROR: %s\n", msg);
	fflush(stdout);
	exit(EXIT_FAILURE);
}

const char *enumtostr(enum ntype typ)
{
	switch (typ) {
	case N_SIMPLEDEF: return "N_SIMPLEDEF"; break;
	case N_VECDEF: return "N_VECDEF"; break;
	case N_FUNCDEF: return "N_FUNCDEF"; break;
	case N_AUTO: return "N_AUTO"; break;
	case N_EXTRN: return "N_EXTRN"; break;
	case N_LABEL: return "N_LABEL"; break;
	case N_CASE: return "N_CASE"; break;
	case N_COMPOUND: return "N_COMPOUND"; break;
	case N_IF: return "N_IF"; break;
	case N_WHILE: return "N_WHILE"; break;
	case N_SWITCH: return "N_SWITCH"; break;
	case N_GOTO: return "N_GOTO"; break;
	case N_RETURN: return "N_RETURN"; break;
	case N_EXPRESSION: return "N_EXPRESSION"; break;
	case N_COMMA: return "N_COMMA"; break;
	case N_ASSIGN: return "N_ASSIGN"; break;
	case N_MUL_ASSIGN: return "N_MUL_ASSIGN"; break;
	case N_DIV_ASSIGN: return "N_DIV_ASSIGN"; break;
	case N_MOD_ASSIGN: return "N_MOD_ASSIGN"; break;
	case N_ADD_ASSIGN: return "N_ADD_ASSIGN"; break;
	case N_SUB_ASSIGN: return "N_SUB_ASSIGN"; break;
	case N_LEFT_ASSIGN: return "N_LEFT_ASSIGN"; break;
	case N_RIGHT_ASSIGN: return "N_RIGHT_ASSIGN"; break;
	case N_AND_ASSIGN: return "N_AND_ASSIGN"; break;
	case N_XOR_ASSIGN: return "N_XOR_ASSIGN"; break;
	case N_OR_ASSIGN: return "N_OR_ASSIGN"; break;
	case N_EQ_ASSIGN: return "N_EQ_ASSIGN"; break;
	case N_NE_ASSIGN: return "N_NE_ASSIGN"; break;
	case N_COND: return "N_COND"; break;
	case N_IOR: return "N_IOR"; break;
	case N_XOR: return "N_XOR"; break;
	case N_AND: return "N_AND"; break;
	case N_EQ: return "N_EQ"; break;
	case N_NE: return "N_NE"; break;
	case N_LT: return "N_LT"; break;
	case N_GT: return "N_GT"; break;
	case N_LE: return "N_LE"; break;
	case N_GE: return "N_GE"; break;
	case N_LEFT: return "N_LEFT"; break;
	case N_RIGHT: return "N_RIGHT"; break;
	case N_ADD: return "N_ADD"; break;
	case N_SUB: return "N_SUB"; break;
	case N_MUL: return "N_MUL"; break;
	case N_DIV: return "N_DIV"; break;
	case N_MOD: return "N_MOD"; break;
	case N_INDIR: return "N_INDIR"; break;
	case N_ADDR: return "N_ADDR"; break;
	case N_NEG: return "N_NEG"; break;
	case N_NOT: return "N_NOT"; break;
	case N_INDEX: return "N_INDEX"; break;
	case N_CALL: return "N_CALL"; break;
	case N_PREINC: return "N_PREINC"; break;
	case N_PREDEC: return "N_PREDEC"; break;
	case N_POSTINC: return "N_POSTINC"; break;
	case N_POSTDEC: return "N_POSTDEC"; break;
	case N_NAME: return "N_NAME"; break;
	case N_CONST: return "N_CONST"; break;
	case N_IVALS: return "N_IVALS"; break;
	case N_STATEMENTS: return "N_STATEMENTS"; break;
	case N_DEFS: return "N_DEFS"; break;
	case N_INITS: return "N_INITS"; break;
	case N_INIT: return "N_INIT"; break;
	case N_NAMES: return "N_NAMES"; break;
	case N_ARGS: return "N_ARGS"; break;
	default: return "!!! UNKNOWN TYPE !!!"; break;
	}

}

void dump_ast(struct node *ast) {
	static int indent = 0;
	int i;

	for (i = 0; i < indent; i++)
		putchar('\t');
	indent++;

	if (!ast) {
		puts("NULL\n");
		return;
	} else {
		printf("%s", enumtostr(ast->typ));
		switch (ast->typ) {
		case N_NAME:
		case N_CONST:
			printf(" (%s)\n", ast->one.val);
			break;
		default:
			putchar('\n');
			dump_ast(ast->one.ast);
			dump_ast(ast->two.ast);
			dump_ast(ast->three.ast);
			break;
		}
	}
	indent--;

}


static LLVMValueRef myvar = NULL;
static LLVMBasicBlockRef mylabel = NULL;
LLVMValueRef codegen(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_label(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	mylabel = LLVMAppendBasicBlock(parent, "label");
	return codegen(ast->one.ast, module, builder);
}

LLVMValueRef codegen_goto(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	return LLVMBuildBr(builder, mylabel);
}

LLVMValueRef codegen_addr(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	/* TODO: Function pointers? */
	return LLVMBuildPtrToInt(builder, myvar, LLVMInt64Type(), "addr");
}

LLVMValueRef codegen_indir(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	/* TODO: Different semantics if being assigned to? */

	return LLVMBuildLoad(builder,
		LLVMBuildIntToPtr(
			builder,
			codegen(ast->one.ast, module, builder),
			LLVMPointerType(LLVMInt64Type(), 0),
			"indir"),
		"loadtmp");

}

LLVMValueRef codegen_while(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef condition, zero, func, body_value;
	LLVMBasicBlockRef body_block, end;

	condition = codegen(ast->one.ast, module, builder);
	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "ifcond");
	/* TODO: func isn't always a func... rename to "block" */
	func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
	body_block = LLVMAppendBasicBlock(func, "body");
	end = LLVMAppendBasicBlock(func, "end");

	LLVMBuildCondBr(builder, condition, body_block, end);
	LLVMPositionBuilderAtEnd(builder, body_block);
	/* TODO: I don't think we need to collect values from then/else blocks */
	body_value = codegen(ast->two.ast, module, builder);

	LLVMBuildBr(builder, end);
	/* TODO: What's the point of this? */
	body_block = LLVMGetInsertBlock(builder);

	LLVMPositionBuilderAtEnd(builder, end);

	return body_value;
}

LLVMValueRef codegen_if(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef zero, condition, parent, ref;
	LLVMBasicBlockRef then_block, else_block, end;

	zero = LLVMConstInt(LLVMInt1Type(), 0, 0);
	condition = codegen(ast->one.ast, module, builder);
	condition = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "tmp_cond");
	parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	then_block = LLVMAppendBasicBlock(parent, "then");
	else_block = LLVMAppendBasicBlock(parent, "else");
	end = LLVMAppendBasicBlock(parent, "end");
	LLVMBuildCondBr(builder, condition, then_block, else_block);

	LLVMPositionBuilderAtEnd(builder, then_block);
	codegen(ast->two.ast, module, builder);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, else_block);
	codegen(ast->three.ast, module, builder);
	ref = LLVMBuildBr(builder, end);

	LLVMPositionBuilderAtEnd(builder, end);
	return ref;
}

LLVMValueRef codegen_lt(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	return LLVMBuildICmp(builder,
		LLVMIntSLT,
		codegen(ast->one.ast, module, builder),
		codegen(ast->two.ast, module, builder),
		"lttmp");
}

LLVMValueRef codegen_add(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	return LLVMBuildAdd(builder,
		codegen(ast->one.ast, module, builder),
		codegen(ast->two.ast, module, builder),
		"addtmp");
}

LLVMValueRef codegen_auto(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	/*
	* TODO: store name to indicate it was initialized.
	* also set up vector initialization.
	* Warn when using unitialized var
	*/
	printf("Alloca'ing %s\n", ast->one.ast->one.ast->one.val);
	myvar = LLVMBuildAlloca(builder, LLVMInt64Type(), ast->one.ast->one.ast->one.val);
	return myvar;
}

LLVMValueRef codegen_name(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	/* TODO: Retrieve variables by name */
	if (!myvar)
		generror("Wuh oh, attempted to access unitialized variable");
	printf("Building loader for %s\n", ast->one.val);
	printf("@@@@@@@@\n");
	LLVMDumpValue(myvar);
	printf("@@@@@@@@\n");
	LLVMValueRef tmp = LLVMBuildLoad(builder, myvar, ast->one.val);
	printf("########\n");
	return tmp;

}

LLVMValueRef codegen_assign(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef result = codegen(ast->two.ast, module, builder);
	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_add_assign(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef result;
	LLVMValueRef lhs = codegen(ast->one.ast, module, builder);
	LLVMValueRef rhs = codegen(ast->two.ast, module, builder);


	if (lhs == NULL || rhs == NULL) {
		return NULL;
	}

	result = LLVMBuildAdd(builder, lhs, rhs, "addtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_postdec(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef result;
	result = LLVMBuildSub(builder,
		codegen(ast->one.ast, module, builder),
		LLVMConstInt(LLVMInt64Type(), 1, 0),
		"subtmp");

	LLVMBuildStore(builder,
		result,
		myvar);

	return result;
}

LLVMValueRef codegen_predec(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMValueRef tmp = myvar;
	myvar = LLVMBuildSub(builder,
		codegen(ast->one.ast, module, builder),
		LLVMConstInt(LLVMInt64Type(), -1, 0),
		"subtmp");

	return tmp;
}

LLVMValueRef codegen_const(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	/* TODO: Handle strings, multichars, escape sequences, and octal ints */
	if (ast->one.val[0] == '"')
		return NULL;
	else if (ast->one.val[0] == '\'')
		return LLVMConstInt(LLVMInt64Type(), ast->one.val[1], 0);
	else
		return LLVMConstIntOfString(LLVMInt64Type(), ast->one.val, 10);
}


LLVMValueRef codegen_statements(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	codegen(ast->one.ast, module, builder);
	return codegen(ast->two.ast, module, builder);
}


LLVMValueRef codegen_call(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
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
	*args = codegen(ast->two.ast, module, builder);

	return LLVMBuildCall(builder, func, args, 1, "calltmp");
}

LLVMValueRef codegen_extrn(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMTypeRef sig;
	LLVMValueRef func;

	sig = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
	func = LLVMAddGlobal(module, sig, "putchar");
	LLVMSetLinkage(func, LLVMExternalLinkage);
	return func;
}

LLVMValueRef codegen_funcdef(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	LLVMTypeRef sig;
	LLVMValueRef func, body;
	LLVMBasicBlockRef block;

	/* TODO: Check if function already defined */
	sig = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
	func = LLVMAddFunction(module, ast->one.ast->one.val, sig);
	LLVMSetLinkage(func, LLVMExternalLinkage);

	block = LLVMAppendBasicBlock(func, "entry");
	LLVMPositionBuilderAtEnd(builder, block);

	body = codegen(ast->three.ast, module, builder);


	LLVMBuildRetVoid(builder);

	if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
		LLVMDeleteFunction(func);
		return NULL;
	}

	return func;
}


LLVMValueRef codegen(struct node *ast, LLVMModuleRef module, LLVMBuilderRef builder)
{
	if (!ast) {
		return NULL;
		//generror("Passed NULL to codegen");
	}

	printf("...compiling %s\n", enumtostr(ast->typ));

	/* TODO: Remove all breaks */
	/* TODO: Store function ptrs in LLYVAL rather than enum??? */
	switch (ast->typ) {
	case N_DEFS:
		return codegen(ast->one.ast, module, builder);
		return codegen(ast->two.ast, module, builder);
		break;
	case N_SIMPLEDEF:
		break;
	case N_VECDEF:
		break;
	case N_FUNCDEF:
		return codegen_funcdef(ast, module, builder);
		break;
	case N_IVALS:
		break;
	case N_STATEMENTS:
		return codegen_statements(ast, module, builder);
		break;
	case N_AUTO:
		return codegen_auto(ast, module, builder);
		break;
	case N_EXTRN:
		return codegen_extrn(ast, module, builder);
		break;
	case N_LABEL:
		return codegen_label(ast, module, builder);
		break;
	case N_CASE:
		break;
	/* TODO: Remove N_COMPOUND from grammar */
	case N_COMPOUND:
		return codegen(ast->one.ast, module, builder);
		break;
	case N_IF:
		return codegen_if(ast, module, builder);
		break;
	case N_WHILE:
		return codegen_while(ast, module, builder);
		break;
	case N_SWITCH:
		break;
	case N_GOTO:
		return codegen_goto(ast, module, builder);
		break;
	case N_RETURN:
		break;
	case N_EXPRESSION:
		break;
	case N_INITS:
		break;
	case N_INIT:
		break;
	case N_NAMES:

		break;
	case N_COMMA:
		break;
	case N_ASSIGN:
		return codegen_assign(ast, module, builder);
		break;
	case N_MUL_ASSIGN:
		break;
	case N_DIV_ASSIGN:
		break;
	case N_MOD_ASSIGN:
		break;
	case N_ADD_ASSIGN:
		return codegen_add_assign(ast, module, builder);
		break;
	case N_SUB_ASSIGN:
		break;
	case N_LEFT_ASSIGN:
		break;
	case N_RIGHT_ASSIGN:
		break;
	case N_AND_ASSIGN:
		break;
	case N_XOR_ASSIGN:
		break;
	case N_OR_ASSIGN:
		break;
	case N_EQ_ASSIGN:
		break;
	case N_NE_ASSIGN:
		break;
	case N_COND:
		break;
	case N_IOR:
		break;
	case N_XOR:
		break;
	case N_AND:
		break;
	case N_EQ:
		break;
	case N_NE:
		break;
	case N_LT:
		return codegen_lt(ast, module, builder);
		break;
	case N_GT:
		break;
	case N_LE:
		break;
	case N_GE:
		break;
	case N_LEFT:
		break;
	case N_RIGHT:
		break;
	case N_ADD:
		return codegen_add(ast, module, builder);
		break;
	case N_SUB:
		break;
	case N_MUL:
		break;
	case N_DIV:
		break;
	case N_MOD:
		break;
	case N_INDIR:
		return codegen_indir(ast, module, builder);
		break;
	case N_ADDR:
		return codegen_addr(ast, module, builder);
		break;
	case N_NEG:
		break;
	case N_NOT:
		break;
	case N_PREINC:
		break;
	case N_PREDEC:
		break;
	case N_INDEX:
		break;
	case N_CALL:
		return codegen_call(ast, module, builder);
		break;
	case N_POSTINC:
		break;
	case N_POSTDEC:
		return codegen_postdec(ast, module, builder);
		break;
	case N_ARGS:
		break;
	case N_NAME:
		return codegen_name(ast, module, builder);
		break;
	case N_CONST:
		return codegen_const(ast, module, builder);
		break;
	default:
		printf("Unhandled top-level type: %s\n", enumtostr(ast->typ));
		generror("Stopping");
		break;
	}

	generror("This should never happen.  Remove once all cases return something");
	return NULL;
}

/* TODO: rename struct node to ast_node, rename arg ast to node */
void compile(struct node *ast)
{
	/* TODO: Free module, define "dbc" as constant */
	/* TODO: Make builder/module static in codegen so we don't have to pass them around */
	static LLVMBuilderRef builder;
	static LLVMModuleRef module;

	builder = LLVMCreateBuilder();
	module = LLVMModuleCreateWithName("dbc");


	LLVMValueRef foo = (codegen(ast, module, builder));
	printf("\n====================================\n");
	LLVMDumpValue(foo);
	printf("====================================\n");

	if (LLVMWriteBitcodeToFile(module, "cgram.bc") != 0) {
		generror("Failed to write bitcode");
	}

	/*
	char *msg;
	if(LLVMCreateExecutionEngineForModule(&engine, module, &msg) != 0) {
		printf("We got a failure...\n");
		LLVMDisposeMessage(msg);
		generror(msg);
	}
	void *fp = LLVMGetPointerToGlobal(engine, foo);
	void (*FP)() = (void (*)())(intptr_t)fp;
	FP();
	*/
}

void free_tree(struct node *ast)
{
	/* TODO:  Free while compiling instead? */
}

struct node *leaf(int type, char *value)
{
	struct node *nleaf = malloc(sizeof(struct node));
	nleaf->typ = type;
	nleaf->one.val = value;
	nleaf->two.ast = NULL;
	nleaf->three.ast = NULL;

	return nleaf;
}

struct node *node3(int type, struct node *one, struct node *two, struct node *three)
{
	/* TODO: Error handling */
	struct node *nnode = malloc(sizeof(struct node));

	nnode->typ = type;
	nnode->one.ast = one;
	nnode->two.ast = two;
	nnode->three.ast = three;

	return nnode;
}

struct node *node2(int type, struct node *one, struct node *two)
{
	return node3(type, one, two, NULL);
}

struct node *node1(int type, struct node *one)
{
	return node3(type, one, NULL, NULL);
}

struct node *node0(int type)
{
	return node3(type, NULL, NULL, NULL);
}
