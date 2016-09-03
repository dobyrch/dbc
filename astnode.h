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

typedef LLVMValueRef (codegen_func)(struct node *);

struct node {
	codegen_func *codegen;
	char *val;
	struct node *one, *two, *three;
};

LLVMValueRef codegen(struct node *);

struct node *leafnode(codegen_func, char *value);
struct node *node0(codegen_func);
struct node *node1(codegen_func, struct node *one);
struct node *node2(codegen_func, struct node *one, struct node *two);
struct node *node3(codegen_func, struct node *one, struct node *two, struct node *three);
