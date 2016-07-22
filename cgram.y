%{
#include <stddef.h>
#include <llvm-c/Core.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include "types.h"
#include "external.h"
%}

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

%%

program
	: definitions
		{ compile($1); free_tree($1); }
	| %empty
		{ yyerror("Empty program\n"); }
	;

definitions
	: definitions definition
		{ $$ = node2(codegen_defs, $1, $2); }
	| definition
	;

definition
	: simple_definition
	| vector_definition
	| function_definition
	;

simple_definition
	: NAME ';'
		{ $$ = node1(codegen_simpledef, leaf(codegen_name, $1)); }
	| NAME  ival_list ';'
		{ $$ = node2(codegen_simpledef, leaf(codegen_name, $1), $2); }
	;

vector_definition
	: NAME '[' ']' ';'
		{ $$ = node3(codegen_vecdef, leaf(codegen_name, $1), NULL, NULL); }
	| NAME '[' ']' ival_list ';'
		{ $$ = node3(codegen_vecdef, leaf(codegen_name, $1), NULL, $4); }
	| NAME '[' constant ']' ';'
		{ $$ = node3(codegen_vecdef, leaf(codegen_name, $1), $3, NULL); }
	| NAME '[' constant ']' ival_list ';'
		{ $$ = node3(codegen_vecdef, leaf(codegen_name, $1), $3, $5); }
	;

function_definition
	: NAME '(' ')' statement
		{ $$ = node3(codegen_funcdef, leaf(codegen_name, $1), NULL, $4); }
	| NAME '(' name_list ')' statement
		{ $$ = node3(codegen_funcdef, leaf(codegen_name, $1), $3, $5); }
	;

ival_list
	: ival_list ',' ival
		{ $$ = node2(codegen_ivals, $1, $3); }
	| ival
		/* TODO: decide on consistent ordering for lists */
		{ $$ = node2(codegen_ivals, NULL, $1); }
	;

ival
	: constant
	| NAME
		{ $$ = leaf(codegen_name, $1); }
	;

statement_list
	: statement_list statement
		{ $$ = node2(codegen_statements, $1, $2); }
	| statement
	;

statement
	: AUTO init_list ';'
		{ $$ = node1(codegen_auto, $2); }
	| EXTRN name_list ';'
		{ $$ = node1(codegen_extrn, $2); }
	| NAME ':' statement
		{ $$ = node2(codegen_label, leaf(codegen_name, $1), $3); }
	| CASE constant ':' statement
		{ $$ = node2(codegen_case, $2, $4); }

	| '{' '}'
		{ $$ = node0(codegen_compound); }
	| '{' statement_list '}'
		{ $$ = node1(codegen_compound, $2); }

	| IF '(' expression ')' statement
		{ $$ = node2(codegen_if, $3, $5); }
	| IF '(' expression ')' statement ELSE statement
		{ $$ = node3(codegen_if, $3, $5, $7); }

	| WHILE '(' expression ')' statement
		{ $$ = node2(codegen_while, $3, $5); }
	| SWITCH '(' expression ')' statement
		{ $$ = node2(codegen_switch, $3, $5); }
	| GOTO NAME ';'
		{ $$ = node1(codegen_goto, leaf(codegen_name, $2)); }

	| RETURN ';'
		{ $$ = node0(codegen_return); }
	| RETURN '(' expression ')' ';'
		{ $$ = node1(codegen_return, $3); }

	| expression ';'
	| ';'
		{ $$ = node0(codegen_expression); }
	;

init_list
	: init_list ',' init
		{ $$ = node2(codegen_inits, $1, $3); }
	| init
	;

init
	: NAME
		{ $$ = node1(codegen_init, leaf(codegen_name, $1)); }
	| NAME constant
		{ $$ = node2(codegen_init, leaf(codegen_name, $1), $2); }
	;

name_list
	: name_list ',' NAME
		{ $$ = node2(codegen_names, $1, leaf(codegen_name, $3)); }
	| NAME
		{ $$ = leaf(codegen_name, $1); }
	;

expression
	: expression ',' assignment_expression
		{ $$ = node2(codegen_comma, $3, $1); }
	| assignment_expression
	;

assignment_expression
	: unary_expression '=' assignment_expression
		{ $$ = node2(codegen_assign, $1, $3); }
	| unary_expression MUL_ASSIGN assignment_expression
		{ $$ = node2(codegen_mul_assign, $1, $3); }
	| unary_expression DIV_ASSIGN assignment_expression
		{ $$ = node2(codegen_div_assign, $1, $3); }
	| unary_expression MOD_ASSIGN assignment_expression
		{ $$ = node2(codegen_mod_assign, $1, $3); }
	| unary_expression ADD_ASSIGN assignment_expression
		{ $$ = node2(codegen_add_assign, $1, $3); }
	| unary_expression SUB_ASSIGN assignment_expression
		{ $$ = node2(codegen_sub_assign, $1, $3); }
	| unary_expression LEFT_ASSIGN assignment_expression
		{ $$ = node2(codegen_left_assign, $1, $3); }
	| unary_expression RIGHT_ASSIGN assignment_expression
		{ $$ = node2(codegen_right_assign, $1, $3); }
	| unary_expression AND_ASSIGN assignment_expression
		{ $$ = node2(codegen_and_assign, $1, $3); }
	| unary_expression XOR_ASSIGN assignment_expression
		{ $$ = node2(codegen_xor_assign, $1, $3); }
	| unary_expression OR_ASSIGN assignment_expression
		{ $$ = node2(codegen_or_assign, $1, $3); }
	| unary_expression EQ_ASSIGN assignment_expression
		{ $$ = node2(codegen_eq_assign, $1, $3); }
	| unary_expression NE_ASSIGN assignment_expression
		{ $$ = node2(codegen_ne_assign, $1, $3); }
	| conditional_expression
	;

conditional_expression
	: inclusive_or_expression '?' expression ':' conditional_expression
		{ $$ = node3(codegen_cond, $1, $3, $5); }
	| inclusive_or_expression
	;

inclusive_or_expression
	: inclusive_or_expression '|' exclusive_or_expression
		{ $$ = node2(codegen_ior, $1, $3); }
	| exclusive_or_expression
	;

exclusive_or_expression
	: exclusive_or_expression '^' and_expression
		{ $$ = node2(codegen_xor, $1, $3); }
	| and_expression
	;

and_expression
	: and_expression '&' equality_expression
		{ $$ = node2(codegen_and, $1, $3); }
	| equality_expression
	;

equality_expression
	: equality_expression EQ_OP relational_expression
		{ $$ = node2(codegen_eq, $1, $3); }
	| equality_expression NE_OP relational_expression
		{ $$ = node2(codegen_ne, $1, $3); }
	| relational_expression
	;

relational_expression
	: relational_expression '<' shift_expression
		{ $$ = node2(codegen_lt, $1, $3); }
	| relational_expression '>' shift_expression
		{ $$ = node2(codegen_gt, $1, $3); }
	| relational_expression LE_OP shift_expression
		{ $$ = node2(codegen_le, $1, $3); }
	| relational_expression GE_OP shift_expression
		{ $$ = node2(codegen_ge, $1, $3); }
	| shift_expression
	;

shift_expression
	: shift_expression LEFT_OP additive_expression
		{ $$ = node2(codegen_left, $1, $3); }
	| shift_expression RIGHT_OP additive_expression
		{ $$ = node2(codegen_right, $1, $3); }
	| additive_expression
	;

additive_expression
	: additive_expression '+' multiplicative_expression
		{ $$ = node2(codegen_add, $1, $3); }
	| additive_expression '-' multiplicative_expression
		{ $$ = node2(codegen_sub, $1, $3); }
	| multiplicative_expression
	;

multiplicative_expression
	: multiplicative_expression '*' unary_expression
		{ $$ = node2(codegen_mul, $1, $3); }
	| multiplicative_expression '/' unary_expression
		{ $$ = node2(codegen_div, $1, $3); }
	| multiplicative_expression '%' unary_expression
		{ $$ = node2(codegen_mod, $1, $3); }
	| unary_expression
	;

unary_expression
	: '*' unary_expression
		{ $$ = node1(codegen_indir, $2); }
	| '&' unary_expression
		{ $$ = node1(codegen_addr, $2); }
	| '-' unary_expression
		{ $$ = node1(codegen_neg, $2); }
	| '!' unary_expression
		{ $$ = node1(codegen_not, $2); }
	| INC_OP unary_expression
		{ $$ = node1(codegen_preinc, $2); }
	| DEC_OP unary_expression
		{ $$ = node1(codegen_predec, $2); }
	| postfix_expression
	;

postfix_expression
	: postfix_expression '[' expression ']'
		{ $$ = node2(codegen_index, $1, $3); }
	| postfix_expression '(' ')'
		{ $$ = node1(codegen_call, $1); }
	| postfix_expression '(' argument_expression_list ')'
		{ $$ = node2(codegen_call, $1, $3); }
	| postfix_expression INC_OP
		{ $$ = node1(codegen_postinc, $1); }
	| postfix_expression DEC_OP
		{ $$ = node1(codegen_postdec, $1); }
	| primary_expression
	;

argument_expression_list
	: argument_expression_list ',' assignment_expression
		{ $$ = node2(codegen_args, $1, $3); }
	| assignment_expression
	;

primary_expression
	: NAME
		{ $$ = leaf(codegen_name, $1); }
	| constant
	| '(' expression ')'
		{ $$ = $2; }
	;

constant
	: LITERAL
		{ $$ = leaf(codegen_const, $1); }
	| STRING_LITERAL
		{ $$ = leaf(codegen_const, $1); }
	;

%%

int main(void)
{
	return yyparse();
}
