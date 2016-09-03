/*
 * Copyright 2016 Douglas Christman
 *
 * This file is part of dbc.
 *
 * dbc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <llvm-c/Core.h>
#include "y.tab.h"
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
