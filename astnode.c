#include <stdlib.h>
#include <llvm-c/Core.h>
#include "dbc.tab.h"
#include "astnode.h"

LLVMValueRef codegen(struct node *ast)
{
	return ast->codegen(ast);
}

struct node *leafnode(codegen_func codegen, char *value)
{
	/* TODO: Error handling */
	struct node *new_leaf = malloc(sizeof(struct node));

	new_leaf->codegen = codegen;
	new_leaf->val = value;
	new_leaf->one = NULL;
	new_leaf->two = NULL;
	new_leaf->three = NULL;

	return new_leaf;
}

struct node *node0(codegen_func codegen)
{
	return node3(codegen, NULL, NULL, NULL);
}

struct node *node1(codegen_func codegen, struct node *one)
{
	return node3(codegen, one, NULL, NULL);
}

struct node *node2(codegen_func codegen, struct node *one, struct node *two)
{
	return node3(codegen, one, two, NULL);
}

struct node *node3(codegen_func codegen, struct node *one, struct node *two, struct node *three)
{
	/* TODO: Error handling */
	struct node *new_node = malloc(sizeof(struct node));

	new_node->codegen = codegen;
	new_node->val = NULL;
	new_node->one = one;
	new_node->two = two;
	new_node->three = three;

	return new_node;
}
