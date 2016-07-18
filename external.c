#include "external.h"
#include <stdlib.h>
#include <stdio.h>

void yyerror(const char *msg)
{
	printf("\n%*s\n%*s\n", column, "^", column, msg);
	fflush(stdout);
}

void compile(struct node *ast)
{
	static int tabs = 0;
	int i;

	if (ast == NULL)
		return;

	for (i = 0; i < tabs; i++)
		printf("\t");
	tabs++;

	printf("(0x%x)", ast);
	switch (ast->typ) {
	case N_CONST:		printf("N_CONST\t\t"); goto named;
	case N_NAME:		printf("N_NAME\t\t"); goto named;
	case N_SIMPLEDEF:	printf("N_SIMPLEDEF\t\t"); goto named;
	case N_FUNCDEF:		printf("N_FUNCDEF\t\t"); goto named;
	case N_VECDEF:		printf("N_VECDEF\t\t"); goto named;
	case N_LABEL:		printf("N_LABEL\t\t"); goto named;
	case N_GOTO:		printf("N_GOTO\t\t"); goto named;
named:
		printf("LEAF: %s\n", ast->one.val);
		/* TODO: remember to free yytext dup */
		compile(ast->two.ast);
		compile(ast->three.ast);
		break;

	case N_AUTO:		printf("N_AUTO\t\t"); goto inspect;
	case N_EXTRN:		printf("N_EXTRN\t\t"); goto inspect;
	case N_CASE:		printf("N_CASE\t\t"); goto inspect;
	case N_COMPOUND:	printf("N_COMPOUND\t"); goto inspect;
	case N_IF:		printf("N_IF\t\t"); goto inspect;
	case N_WHILE:		printf("N_WHILE\t\t"); goto inspect;
	case N_SWITCH:		printf("N_SWITCH\t\t"); goto inspect;
	case N_RETURN:		printf("N_RETURN\t\t"); goto inspect;
	case N_COMMA:		printf("N_COMMA\t\t"); goto inspect;
	case N_ASSIGN:		printf("N_ASSIGN\t\t"); goto inspect;
	case N_MUL_ASSIGN:	printf("N_MUL_ASSIGN\t"); goto inspect;
	case N_DIV_ASSIGN:	printf("N_DIV_ASSIGN\t"); goto inspect;
	case N_MOD_ASSIGN:	printf("N_MOD_ASSIGN\t"); goto inspect;
	case N_ADD_ASSIGN:	printf("N_ADD_ASSIGN\t"); goto inspect;
	case N_SUB_ASSIGN:	printf("N_SUB_ASSIGN\t"); goto inspect;
	case N_LEFT_ASSIGN:	printf("N_LEFT_ASSIGN\t"); goto inspect;
	case N_RIGHT_ASSIGN:	printf("N_RIGHT_ASSIGN\t"); goto inspect;
	case N_AND_ASSIGN:	printf("N_AND_ASSIGN\t"); goto inspect;
	case N_XOR_ASSIGN:	printf("N_XOR_ASSIGN\t"); goto inspect;
	case N_OR_ASSIGN:	printf("N_OR_ASSIGN\t"); goto inspect;
	case N_EQ_ASSIGN:	printf("N_EQ_ASSIGN\t"); goto inspect;
	case N_NE_ASSIGN:	printf("N_NE_ASSIGN\t"); goto inspect;
	case N_COND:		printf("N_COND\t\t"); goto inspect;
	case N_IOR:		printf("N_IOR\t\t"); goto inspect;
	case N_XOR:		printf("N_XOR\t\t"); goto inspect;
	case N_AND:		printf("N_AND\t\t"); goto inspect;
	case N_EQ:		printf("N_EQ\t\t"); goto inspect;
	case N_NE:		printf("N_NE\t\t"); goto inspect;
	case N_LT:		printf("N_LT\t\t"); goto inspect;
	case N_GT:		printf("N_GT\t\t"); goto inspect;
	case N_LE:		printf("N_LE\t\t"); goto inspect;
	case N_GE:		printf("N_GE\t\t"); goto inspect;
	case N_LEFT:		printf("N_LEFT\t\t"); goto inspect;
	case N_RIGHT:		printf("N_RIGHT\t\t"); goto inspect;
	case N_ADD:		printf("N_ADD\t\t"); goto inspect;
	case N_SUB:		printf("N_SUB\t\t"); goto inspect;
	case N_MUL:		printf("N_MUL\t\t"); goto inspect;
	case N_DIV:		printf("N_DIV\t\t"); goto inspect;
	case N_MOD:		printf("N_MOD\t\t"); goto inspect;
	case N_INDIR:		printf("N_INDIR\t\t"); goto inspect;
	case N_ADDR:		printf("N_ADDR\t\t"); goto inspect;
	case N_NEG:		printf("N_NEG\t\t"); goto inspect;
	case N_NOT:		printf("N_NOT\t\t"); goto inspect;
	case N_INDEX:		printf("N_INDEX\t\t"); goto inspect;
	case N_CALL:		printf("N_CALL\t\t"); goto inspect;
	case N_PREINC:		printf("N_PREINC\t\t"); goto inspect;
	case N_PREDEC:		printf("N_PREDEC\t\t"); goto inspect;
	case N_POSTINC:		printf("N_POSTINC\t\t"); goto inspect;
	case N_POSTDEC:		printf("N_POSTDEC\t\t"); goto inspect;
	case N_DUMMY:		printf("N_DUMMY\t\t"); goto inspect;
	default:		printf("!!!UNKNOWN TYPE: %x\n", ast->typ); fflush(stdout); goto inspect;

inspect:

		printf("ONE: %x\tTWO: %x\tTHREE: %x\n", ast->one.val, ast->two.val, ast->three.val);
		compile(ast->one.ast);
		compile(ast->two.ast);
		compile(ast->three.ast);
	}

	tabs--;

}

void free_tree(struct node *ast)
{
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
