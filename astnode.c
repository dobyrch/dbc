#include <stdlib.h>

#include <llvm-c/Core.h>

#include "dbc.tab.h"
#include "astnode.h"


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
