%union {
	char *val;
	struct node *ast;
}

%token <val> NAME LITERAL STRING_LITERAL
%token INC_OP DEC_OP
%token LEFT_OP RIGHT_OP
%token LE_OP GE_OP
%token EQ_OP NE_OP
%token AND_OP OR_OP
%token MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token ADD_ASSIGN SUB_ASSIGN
%token LEFT_ASSIGN RIGHT_ASSIGN
%token AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token EQ_ASSIGN NE_ASSIGN
%token CASE IF ELSE SWITCH WHILE GOTO RETURN
%token AUTO EXTRN

%type <ast> definitions definition
%type <ast> simple_definition vector_definition function_definition
%type <ast> ival_list ival
%type <ast> statement_list statement
%type <ast> init_list init
%type <ast> name_list
%type <ast> expression assignment_expression conditional_expression
%type <ast> inclusive_or_expression exclusive_or_expression and_expression
%type <ast> equality_expression relational_expression
%type <ast> shift_expression additive_expression multiplicative_expression
%type <ast> unary_expression postfix_expression argument_expression_list
%type <ast> primary_expression constant

%define parse.error verbose

%{
#include <stddef.h>
#include <llvm-c/Core.h>
#include "external.h"
%}

%%

program
	: definitions
		{ compile($1); free_tree($1); }
	| %empty
		{ yyerror("Empty program\n"); }
	;

definitions
	: definitions definition
		{ $$ = node2(gen_defs, $1, $2); }
	| definition
	;

definition
	: simple_definition
	| vector_definition
	| function_definition
	;

simple_definition
	: NAME ';'
		{ $$ = node1(gen_simpledef, leaf(gen_name, $1)); }
	| NAME  ival_list ';'
		{ $$ = node2(gen_simpledef, leaf(gen_name, $1), $2); }
	;

vector_definition
	: NAME '[' ']' ';'
		{ $$ = node3(gen_vecdef, leaf(gen_name, $1), NULL, NULL); }
	| NAME '[' ']' ival_list ';'
		{ $$ = node3(gen_vecdef, leaf(gen_name, $1), NULL, $4); }
	| NAME '[' constant ']' ';'
		{ $$ = node3(gen_vecdef, leaf(gen_name, $1), $3, NULL); }
	| NAME '[' constant ']' ival_list ';'
		{ $$ = node3(gen_vecdef, leaf(gen_name, $1), $3, $5); }
	;

function_definition
	: NAME '(' ')' statement
		{ $$ = node3(gen_funcdef, leaf(gen_name, $1), NULL, $4); }
	| NAME '(' name_list ')' statement
		{ $$ = node3(gen_funcdef, leaf(gen_name, $1), $3, $5); }
	;

ival_list
	: ival_list ',' ival
		{ $$ = node2(gen_ivals, $1, $3); }
	| ival
		/* TODO: decide on consistent ordering for lists */
		{ $$ = node2(gen_ivals, NULL, $1); }
	;

ival
	: constant
	| NAME
		{ $$ = leaf(gen_name, $1); }
	;

statement_list
	: statement_list statement
		{ $$ = node2(gen_statements, $1, $2); }
	| statement
	;

statement
	: AUTO init_list ';'
		{ $$ = node1(gen_auto, $2); }
	| EXTRN name_list ';'
		{ $$ = node1(gen_extrn, $2); }
	| NAME ':' statement
		{ $$ = node2(gen_label, leaf(gen_name, $1), $3); }
	| CASE constant ':' statement
		{ $$ = node2(gen_case, $2, $4); }

	| '{' '}'
		{ $$ = node0(gen_compound); }
	| '{' statement_list '}'
		{ $$ = node1(gen_compound, $2); }

	| IF '(' expression ')' statement
		{ $$ = node2(gen_if, $3, $5); }
	| IF '(' expression ')' statement ELSE statement
		{ $$ = node3(gen_if, $3, $5, $7); }

	| WHILE '(' expression ')' statement
		{ $$ = node2(gen_while, $3, $5); }
	| SWITCH '(' expression ')' statement
		{ $$ = node2(gen_switch, $3, $5); }
	| GOTO NAME ';'
		{ $$ = node1(gen_goto, leaf(gen_name, $2)); }

	| RETURN ';'
		{ $$ = node0(gen_return); }
	| RETURN '(' expression ')' ';'
		{ $$ = node1(gen_return, $3); }

	| expression ';'
	| ';'
		{ $$ = node0(gen_expression); }
	;

init_list
	: init_list ',' init
		{ $$ = node2(gen_inits, $1, $3); }
	| init
	;

init
	: NAME
		{ $$ = node1(gen_init, leaf(gen_name, $1)); }
	| NAME constant
		{ $$ = node2(gen_init, leaf(gen_name, $1), $2); }
	;

name_list
	: name_list ',' NAME
		{ $$ = node2(gen_names, $1, leaf(gen_name, $3)); }
	| NAME
		{ $$ = leaf(gen_name, $1); }
	;

expression
	: expression ',' assignment_expression
		{ $$ = node2(gen_comma, $3, $1); }
	| assignment_expression
	;

assignment_expression
	: unary_expression '=' assignment_expression
		{ $$ = node2(gen_assign, $1, $3); }
	| unary_expression MUL_ASSIGN assignment_expression
		{ $$ = node2(gen_mul_assign, $1, $3); }
	| unary_expression DIV_ASSIGN assignment_expression
		{ $$ = node2(gen_div_assign, $1, $3); }
	| unary_expression MOD_ASSIGN assignment_expression
		{ $$ = node2(gen_mod_assign, $1, $3); }
	| unary_expression ADD_ASSIGN assignment_expression
		{ $$ = node2(gen_add_assign, $1, $3); }
	| unary_expression SUB_ASSIGN assignment_expression
		{ $$ = node2(gen_sub_assign, $1, $3); }
	| unary_expression LEFT_ASSIGN assignment_expression
		{ $$ = node2(gen_left_assign, $1, $3); }
	| unary_expression RIGHT_ASSIGN assignment_expression
		{ $$ = node2(gen_right_assign, $1, $3); }
	| unary_expression AND_ASSIGN assignment_expression
		{ $$ = node2(gen_and_assign, $1, $3); }
	| unary_expression XOR_ASSIGN assignment_expression
		{ $$ = node2(gen_xor_assign, $1, $3); }
	| unary_expression OR_ASSIGN assignment_expression
		{ $$ = node2(gen_or_assign, $1, $3); }
	| unary_expression EQ_ASSIGN assignment_expression
		{ $$ = node2(gen_eq_assign, $1, $3); }
	| unary_expression NE_ASSIGN assignment_expression
		{ $$ = node2(gen_ne_assign, $1, $3); }
	| conditional_expression
	;

conditional_expression
	: inclusive_or_expression '?' expression ':' conditional_expression
		{ $$ = node3(gen_cond, $1, $3, $5); }
	| inclusive_or_expression
	;

inclusive_or_expression
	: inclusive_or_expression '|' exclusive_or_expression
		{ $$ = node2(gen_ior, $1, $3); }
	| exclusive_or_expression
	;

exclusive_or_expression
	: exclusive_or_expression '^' and_expression
		{ $$ = node2(gen_xor, $1, $3); }
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
	| relational_expression '>' shift_expression
		{ $$ = node2(gen_gt, $1, $3); }
	| relational_expression LE_OP shift_expression
		{ $$ = node2(gen_le, $1, $3); }
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
	| postfix_expression '(' argument_expression_list ')'
		{ $$ = node2(gen_call, $1, $3); }
	| postfix_expression INC_OP
		{ $$ = node1(gen_postinc, $1); }
	| postfix_expression DEC_OP
		{ $$ = node1(gen_postdec, $1); }
	| primary_expression
	;

argument_expression_list
	: argument_expression_list ',' assignment_expression
		{ $$ = node2(gen_args, $1, $3); }
	| assignment_expression
	;

primary_expression
	: NAME
		{ $$ = leaf(gen_name, $1); }
	| constant
	| '(' expression ')'
		{ $$ = $2; }
	;

constant
	: LITERAL
		{ $$ = leaf(gen_const, $1); }
	| STRING_LITERAL
		{ $$ = leaf(gen_const, $1); }
	;

%%

int main(void)
{
	return yyparse();
}
