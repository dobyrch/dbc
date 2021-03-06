%{
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
%}

%union {
	char *val;
	struct node *ast;
}

%token <val> NAME LITERAL
%token INC_OP DEC_OP
%token LEFT_OP RIGHT_OP
%token LE_OP GE_OP
%token EQ_OP NE_OP
%token MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token ADD_ASSIGN SUB_ASSIGN
%token LEFT_ASSIGN RIGHT_ASSIGN
%token AND_ASSIGN OR_ASSIGN
%token EQ_ASSIGN NE_ASSIGN
%token LT_ASSIGN LE_ASSIGN
%token GT_ASSIGN GE_ASSIGN
%token IF ELSE WHILE
%token SWITCH CASE GOTO RETURN
%token AUTO EXTRN
%token JUNK

%type <ast> definition_list definition
%type <ast> simple_definition vector_definition function_definition
%type <ast> ival_list ival
%type <ast> statement_list statement
%type <ast> expression_statement other_statement
%type <ast> init_list init
%type <ast> name_list
%type <ast> expression conditional_expression
%type <ast> or_expression and_expression
%type <ast> equality_expression relational_expression
%type <ast> shift_expression additive_expression multiplicative_expression
%type <ast> unary_expression postfix_expression argument_list
%type <ast> primary_expression constant

%right THEN ELSE

%{
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include "astnode.h"
#include "codegen.h"

extern int yylex(void);
extern FILE *yyin;

static void yyerror(const char *msg);
static char *outfile;
static int failed = 0;
%}

%%

program
	: definition_list
		{ if (failed) return EXIT_FAILURE;
		  compile($1, outfile); free_tree($1); }
	| /* empty */
		{ compile(NULL, outfile); }
	;

definition_list
	/* TODO: consistently name all code generation functions */
	: definition_list definition
		{ $$ = node2(gen_defs, $2, $1); }
	| definition
		{ $$ = node1(gen_defs, $1); }
	;

definition
	: simple_definition
	| vector_definition
	| function_definition
	;

simple_definition
	: NAME ';'
		{ $$ = node1(gen_simpledef, leafnode(gen_name, $1)); }
	| NAME ival_list ';'
		{ $$ = node2(gen_simpledef, leafnode(gen_name, $1), $2); }
	;

vector_definition
	: NAME '[' ']' ';'
		{ $$ = node3(gen_vecdef, leafnode(gen_name, $1), NULL, NULL); }
	| NAME '[' ']' ival_list ';'
		{ $$ = node3(gen_vecdef, leafnode(gen_name, $1), NULL, $4); }
	| NAME '[' constant ']' ';'
		{ $$ = node3(gen_vecdef, leafnode(gen_name, $1), $3, NULL); }
	| NAME '[' constant ']' ival_list ';'
		{ $$ = node3(gen_vecdef, leafnode(gen_name, $1), $3, $5); }
	;

function_definition
	: NAME '(' ')' statement
		{ $$ = node3(gen_funcdef, leafnode(gen_name, $1), NULL, $4); }
	| NAME '(' name_list ')' statement
		{ $$ = node3(gen_funcdef, leafnode(gen_name, $1), $3, $5); }
	;

ival_list
	: ival_list ',' ival
		{ $$ = node2(gen_ivals, $3, $1); }
	| ival
		{ $$ = node1(gen_ivals, $1); }
	;

ival
	: constant
	| NAME
		{ $$ = leafnode(gen_name, $1); }
	;

statement_list
	: statement_list statement
		{ $$ = node2(gen_statements, $1, $2); }
	| statement
	;

statement
	: expression_statement
	| other_statement
	;

expression_statement
	: expression ';'
	;

other_statement
	: AUTO init_list ';' statement
		{ $$ = node2(gen_auto, $2, $4); }
	| EXTRN name_list ';' statement
		{ $$ = node2(gen_extrn, $2, $4); }
	| NAME ':' statement
		{ $$ = node2(gen_label, leafnode(gen_name, $1), $3); }
	| CASE constant ':' statement
		{ $$ = node2(gen_case, $2, $4); }

	| '{' '}'
		{ $$ = node0(gen_compound); }
	| '{' statement_list '}'
		{ $$ = node1(gen_compound, $2); }

	| IF '(' expression ')' statement %prec THEN
		{ $$ = node2(gen_if, $3, $5); }
	| IF '(' expression ')' statement ELSE statement
		{ $$ = node3(gen_if, $3, $5, $7); }

	| WHILE '(' expression ')' statement
		{ $$ = node2(gen_while, $3, $5); }
	| SWITCH expression other_statement
		{ $$ = node2(gen_switch, $2, $3); }
	| GOTO expression ';'
		{ $$ = node1(gen_goto, $2); }

	| RETURN ';'
		{ $$ = node0(gen_return); }
	| RETURN '(' expression ')' ';'
		{ $$ = node1(gen_return, $3); }

	| ';'
		{ $$ = node0(gen_null); }
	;

/* TODO: rename to something like "name_const_list" to avoid confusion with ival_list */
/* TODO: and remember to change the name in codegen too! */
init_list
	: init_list ',' init
		{ $$ = node2(gen_inits, $3, $1); }
	| init
		{ $$ = node1(gen_inits, $1); }
	;

init
	: NAME
		/*
		 * TODO: Remove unused gen_* funcs:
		 * gen_name is not called in this context
		 * perhaps call "gen_fail" which prints an error
		 */
		{ $$ = node1(gen_init, leafnode(gen_name, $1)); }
	| NAME constant
		{ $$ = node2(gen_init, leafnode(gen_name, $1), $2); }
	;

name_list
	: name_list ',' NAME
		{ $$ = node2(gen_names, leafnode(gen_name, $3), $1); }
	| NAME
		{ $$ = node1(gen_names, leafnode(gen_name, $1)); }
	;

expression
	: unary_expression '=' expression
		{ $$ = node2(gen_assign, $1, $3); }
	| unary_expression MUL_ASSIGN expression
		{ $$ = node2(gen_mul_assign, $1, $3); }
	| unary_expression DIV_ASSIGN expression
		{ $$ = node2(gen_div_assign, $1, $3); }
	| unary_expression MOD_ASSIGN expression
		{ $$ = node2(gen_mod_assign, $1, $3); }
	| unary_expression ADD_ASSIGN expression
		{ $$ = node2(gen_add_assign, $1, $3); }
	| unary_expression SUB_ASSIGN expression
		{ $$ = node2(gen_sub_assign, $1, $3); }
	| unary_expression LEFT_ASSIGN expression
		{ $$ = node2(gen_left_assign, $1, $3); }
	| unary_expression RIGHT_ASSIGN expression
		{ $$ = node2(gen_right_assign, $1, $3); }
	| unary_expression AND_ASSIGN expression
		{ $$ = node2(gen_and_assign, $1, $3); }
	| unary_expression OR_ASSIGN expression
		{ $$ = node2(gen_or_assign, $1, $3); }
	| unary_expression EQ_ASSIGN expression
		{ $$ = node2(gen_eq_assign, $1, $3); }
	| unary_expression NE_ASSIGN expression
		{ $$ = node2(gen_ne_assign, $1, $3); }
	| unary_expression LT_ASSIGN expression
		{ $$ = node2(gen_lt_assign, $1, $3); }
	| unary_expression LE_ASSIGN expression
		{ $$ = node2(gen_le_assign, $1, $3); }
	| unary_expression GT_ASSIGN expression
		{ $$ = node2(gen_gt_assign, $1, $3); }
	| unary_expression GE_ASSIGN expression
		{ $$ = node2(gen_ge_assign, $1, $3); }
	| conditional_expression
	;

conditional_expression
	: or_expression '?' expression ':' conditional_expression
		{ $$ = node3(gen_cond, $1, $3, $5); }
	| or_expression
	;

or_expression
	: or_expression '|' and_expression
		{ $$ = node2(gen_or, $1, $3); }
	| and_expression
	;

and_expression
	: and_expression '&' equality_expression
		{ $$ = node2(gen_and, $1, $3); }
	| equality_expression
	;

equality_expression
	: equality_expression EQ_OP relational_expression
		{ $$ = node2(gen_eq, $1, $3); }
	| equality_expression NE_OP relational_expression
		{ $$ = node2(gen_ne, $1, $3); }
	| relational_expression
	;

relational_expression
	: relational_expression '<' shift_expression
		{ $$ = node2(gen_lt, $1, $3); }
	| relational_expression LE_OP shift_expression
		{ $$ = node2(gen_le, $1, $3); }
	| relational_expression '>' shift_expression
		{ $$ = node2(gen_gt, $1, $3); }
	| relational_expression GE_OP shift_expression
		{ $$ = node2(gen_ge, $1, $3); }
	| shift_expression
	;

shift_expression
	: shift_expression LEFT_OP additive_expression
		{ $$ = node2(gen_left, $1, $3); }
	| shift_expression RIGHT_OP additive_expression
		{ $$ = node2(gen_right, $1, $3); }
	| additive_expression
	;

additive_expression
	: additive_expression '+' multiplicative_expression
		{ $$ = node2(gen_add, $1, $3); }
	| additive_expression '-' multiplicative_expression
		{ $$ = node2(gen_sub, $1, $3); }
	| multiplicative_expression
	;

multiplicative_expression
	: multiplicative_expression '*' unary_expression
		{ $$ = node2(gen_mul, $1, $3); }
	| multiplicative_expression '/' unary_expression
		{ $$ = node2(gen_div, $1, $3); }
	| multiplicative_expression '%' unary_expression
		{ $$ = node2(gen_mod, $1, $3); }
	| unary_expression
	;

unary_expression
	: '*' unary_expression
		{ $$ = node1(gen_indir, $2); }
	| '&' unary_expression
		{ $$ = node1(gen_addr, $2); }
	| '-' unary_expression
		{ $$ = node1(gen_neg, $2); }
	| '!' unary_expression
		{ $$ = node1(gen_not, $2); }
	| INC_OP unary_expression
		{ $$ = node1(gen_preinc, $2); }
	| DEC_OP unary_expression
		{ $$ = node1(gen_predec, $2); }
	| postfix_expression
	;

postfix_expression
	: postfix_expression '[' expression ']'
		{ $$ = node2(gen_index, $1, $3); }
	| postfix_expression '(' ')'
		{ $$ = node1(gen_call, $1); }
	| postfix_expression '(' argument_list ')'
		{ $$ = node2(gen_call, $1, $3); }
	| postfix_expression INC_OP
		{ $$ = node1(gen_postinc, $1); }
	| postfix_expression DEC_OP
		{ $$ = node1(gen_postdec, $1); }
	| primary_expression
	;

argument_list
	: argument_list ',' expression
		{ $$ = node2(NULL, $3, $1); }
	| expression
		{ $$ = node1(NULL, $1); }
	;

primary_expression
	: '(' expression ')'
		{ $$ = $2; }
	| NAME
		{ $$ = leafnode(gen_name, $1); }
	| constant
	;

constant
	: LITERAL
		{ $$ = leafnode(gen_const, $1); }
	;

/* Error Handling Rules */

definition
	: error ';'
		{ yyerror("xx"); }
	| error '{' statement_list '}'
		{ yyerror("xx"); }
	;

other_statement
	: AUTO error ';' statement
		{ yyerror("sx auto"); }
	| EXTRN error ';' statement
		{ yyerror("sx extrn"); }
	| CASE error statement
		{ yyerror("sx case"); }
	| IF error statement %prec THEN
		{ yyerror("sx if"); }
	| IF error statement ELSE statement
		{ yyerror("sx if"); }
	| WHILE error statement
		{ yyerror("sx while"); }
	| RETURN error ';'
		{ yyerror("sx return"); }
	| error ';'
		{ yyerror("ex"); }
	;

%%

void yyerror(const char *msg)
{
	static int last_line = 0;

	failed = 1;

	if (strncmp(msg, "syntax error", 12) == 0) {
		/* TODO: Figure out how to check expected token */
		if (strchr(msg, ')'))
			fprintf(stderr, "%d: ()\n", yylineno);
		else if (strchr(msg, ']'))
			fprintf(stderr, "%d: []\n", yylineno);
		else if (strchr(msg, '}'))
			fprintf(stderr, "%d: $)\n", yylineno);

		last_line = yylineno;
		return;
	}

	if (last_line) {
		yylineno = last_line;
		last_line = 0;
	}

	fprintf(stderr, "%d: %s\n", yylineno, msg);
}

int main(int argc, char **argv)
{
	char *progname;

	progname = basename(argv[0]);

	if (argc != 3) {
		fprintf(stderr, "Usage: %s infile outfile\n", progname);
		exit(EXIT_FAILURE);
	}

	yyin = fopen(argv[1], "r");

	if (!yyin) {
		fprintf(stderr, "%s: %s: %s\n", progname, argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	outfile = argv[2];

	return yyparse();
}
