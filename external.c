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

/* TODO: Free module, define "dbc" as constant */
static LLVMBuilderRef builder;
static LLVMModuleRef module;

void compinit(void)
{
	builder = LLVMCreateBuilder();
	module = LLVMModuleCreateWithName("dbc");
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
	default: return "!!! N_UNKNOWN !!!"; break;
	}

}

void compile(struct node *ast) {
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
			compile(ast->one.ast);
			compile(ast->two.ast);
			compile(ast->three.ast);
			break;
		}
	}
	indent--;

}

void compile_statement(struct node *ast)
{
	if (!ast)
		return;

	switch (ast->typ) {
		case N_COMPOUND:
			if (ast->one.ast) {
				compile_statement(ast->one.ast->one.ast);
				compile_statement(ast->one.ast->two.ast);
			}
			break;
		case N_AUTO:
			printf("Compiling AUTO not yet implemented\n");
			break;
		case N_EXPRESSION:
			printf("Compiling EXPRESSION not yet implemented\n");
			break;
		case N_ASSIGN:
			generror("Got an ASSIGN, expected STATEMENT!\n");
		default:
			generror("Unexpected statement type");
	}
}


void compile_definition(struct node *ast)
{
	switch (ast->typ) {
		case N_SIMPLEDEF:
			break;
		case N_VECDEF:
			break;
		case N_FUNCDEF:
			compile_statement(ast->three.ast);
		default:
			yyerror("Unexpected top-level type");
	}
}

void compile_(struct node *ast)
{
	/*
	if (ast->typ == N_DUMMY) {
		if (ast->one.ast)
			compile(ast->one.ast);

		compile_definition(ast->two.ast);
	} else {
		compile_definition(ast);
	}
	*/
}

void compile_const(struct node *ast)
{
	    LLVMDumpValue(LLVMConstIntOfString(LLVMInt32Type(), ast->one.val, 10));
}

void free_tree(struct node *ast)
{
	/* TODO:  Free while compiling instead? */
	printf("!! Freeing not yet implemented !!\n");
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
