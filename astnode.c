#include <stdlib.h>

#include <llvm-c/Core.h>

#include "dbc.tab.h"
#include "astnode.h"


LLVMValueRef codegen(struct node *ast)
{
	return ast->codegen(ast);
}


char *val(struct node *ast)
{
	return ast->one.val;
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


struct node *leaf(codegen_func codegen, char *value)
{
	/* TODO: Error handling */
	struct node *new_leaf = malloc(sizeof(struct node));

	new_leaf->codegen = codegen;
	new_leaf->one.val = value;
	new_leaf->two.ast = NULL;
	new_leaf->three.ast = NULL;

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
	new_node->one.ast = one;
	new_node->two.ast = two;
	new_node->three.ast = three;

	return new_node;
}
