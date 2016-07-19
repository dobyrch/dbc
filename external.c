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

/* TODO: Free module, define "dbc" as constant */
static LLVMBuilderRef builder;
static LLVMModuleRef module;

void compinit(void)
{
	builder = LLVMCreateBuilder();
	module = LLVMModuleCreateWithName("dbc");
}

void compile(struct node *ast)
{
	if (ast == NULL)
		return;

	switch (ast->typ) {
		case N_CONST:
		    LLVMConstReal(LLVMInt32Type(), atoi(ast->one.val));
		    break;
	}
}

void free_tree(struct node *ast)
{
	/* TODO:  Free while compiling instead? */
	printf("!! Freeing not yet implemented !!\n");
}

struct node *leaf3(int type, char *value, struct node *two, struct node *three)
{
	struct node *nleaf = malloc(sizeof(struct node));
	nleaf->typ = type;
	nleaf->one.val = value;
	nleaf->two.ast = two;
	nleaf->three.ast = three;

	return nleaf;
}

struct node *leaf2(int type, char *value, struct node *two)
{
	return leaf3(type, value, two, NULL);
}

struct node *leaf1(int type, char *value)
{
	return leaf3(type, value, NULL, NULL);
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
