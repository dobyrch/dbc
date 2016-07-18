%{
#include <stddef.h>
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

%type <typ> assignment_operator unary_operator

%%

program
	: definitions	{ compile($1); free_tree($1); }
	| /* empty */	{ yyerror("Empty program\n"); }
	;

definitions
	: definitions definition	{ $$ = node2(N_DUMMY, $1, $2); }
	| definition
	;

definition
	: simple_definition
	| vector_definition
	| function_definition
	;

simple_definition
	: NAME ';'		{ $$ = leaf1(N_SIMPLEDEF, $1); }
	| NAME  ival_list ';'	{ $$ = leaf2(N_SIMPLEDEF, $1, $2); }
	;

vector_definition
	: NAME '[' ']' ';'			{ $$ = leaf3(N_VECDEF, $1, NULL, NULL); }
	| NAME '[' ']' ival_list ';'		{ $$ = leaf3(N_VECDEF, $1, NULL, $4); }
	| NAME '[' constant ']' ';'		{ $$ = leaf3(N_VECDEF, $1, $3, NULL); }
	| NAME '[' constant ']' ival_list ';'	{ $$ = leaf3(N_VECDEF, $1, $3, $5); }
	;

function_definition
	: NAME '(' ')' statement		{ $$ = leaf3(N_FUNCDEF, $1, NULL, $4); }
	| NAME '(' name_list ')' statement	{ $$ = leaf3(N_FUNCDEF, $1, $3, $5); }
	;

ival_list
	: ival_list ',' ival	{ $$ = node2(N_DUMMY, $1, $3); }
	| ival
	;

ival
	: constant
	| NAME		{ $$ = leaf1(N_NAME, $1); }
	;

statement_list
	: statement_list statement	{ $$ = node2(N_DUMMY, $2, $1); }
	| statement
	;

statement
	: AUTO init_list ';'		{ $$ = node1(N_AUTO, $2); }
	| EXTRN name_list ';'		{ $$ = node1(N_EXTRN, $2); }
	| NAME ':' statement		{ $$ = leaf2(N_LABEL, $1, $3); }
	| CASE constant ':' statement	{ $$ = node2(N_CASE, $2, $4); }

	| '{' '}'			{ $$ = node0(N_COMPOUND); }
	| '{' statement_list '}'	{ $$ = node1(N_COMPOUND, $2); }

	| IF '(' expression ')' statement			{ $$ = node2(N_IF, $3, $5); }
	| IF '(' expression ')' statement ELSE statement	{ $$ = node3(N_IF, $3, $5, $7); }

	| WHILE '(' expression ')' statement	{ $$ = node2(N_WHILE, $3, $5); }
	| SWITCH '(' expression ')' statement	{ $$ = node2(N_SWITCH, $3, $5); }
	| GOTO NAME ';'				{ $$ = leaf1(N_GOTO, $2); }

	| RETURN ';'				{ $$ = node0(N_RETURN); }
	| RETURN '(' expression ')' ';'		{ $$ = node1(N_RETURN, $3); }

	| expression ';'
	| ';'					{ $$ = node0(N_DUMMY); }
	;

init_list
	: init_list ',' init	{ $$ = node2(N_DUMMY, $3, $1); }
	| init
	;

init
	: NAME		{ $$ = leaf1(N_DUMMY, $1); }
	| NAME constant	{ $$ = leaf2(N_DUMMY, $1, $2); }
	;

name_list
	: name_list ',' NAME	{ $$ = leaf2(N_DUMMY, $3, $1); }
	| NAME			{ $$ = leaf1(N_DUMMY, $1); }
	;

expression
	: expression ',' assignment_expression	{ $$ = node2(N_COMMA, $3, $1); }
	| assignment_expression
	;

assignment_expression
	: unary_expression assignment_operator assignment_expression	{ $$ = node2($2, $1, $3); }
	| conditional_expression
	;

assignment_operator
	: '='		{ $$ = N_ASSIGN; }
	| MUL_ASSIGN	{ $$ = N_MUL_ASSIGN; }
	| DIV_ASSIGN	{ $$ = N_DIV_ASSIGN; }
	| MOD_ASSIGN	{ $$ = N_MOD_ASSIGN; }
	| ADD_ASSIGN	{ $$ = N_ADD_ASSIGN; }
	| SUB_ASSIGN	{ $$ = N_SUB_ASSIGN; }
	| LEFT_ASSIGN	{ $$ = N_LEFT_ASSIGN; }
	| RIGHT_ASSIGN	{ $$ = N_RIGHT_ASSIGN; }
	| AND_ASSIGN	{ $$ = N_AND_ASSIGN; }
	| XOR_ASSIGN	{ $$ = N_XOR_ASSIGN; }
	| OR_ASSIGN	{ $$ = N_OR_ASSIGN; }
	| EQ_ASSIGN	{ $$ = N_EQ_ASSIGN; }
	| NE_ASSIGN	{ $$ = N_NE_ASSIGN; }
	;

conditional_expression
	: inclusive_or_expression '?' expression ':' conditional_expression	{ $$ = node3(N_COND, $1, $3, $5); }
	| inclusive_or_expression
	;

inclusive_or_expression
	: inclusive_or_expression '|' exclusive_or_expression 	{ $$ = node2(N_IOR, $1, $3); }
	| exclusive_or_expression
	;

exclusive_or_expression
	: exclusive_or_expression '^' and_expression	{ $$ = node2(N_XOR, $1, $3); }
	| and_expression
	;

and_expression
	: and_expression '&' equality_expression	{ $$ = node2(N_AND, $1, $3); }
	| equality_expression
	;

equality_expression
	: equality_expression EQ_OP relational_expression	{ $$ = node2(N_EQ, $1, $3); }
	| equality_expression NE_OP relational_expression	{ $$ = node2(N_NE, $1, $3); }
	| relational_expression
	;

relational_expression
	: relational_expression '<' shift_expression	{ $$ = node2(N_LT, $1, $3); }
	| relational_expression '>' shift_expression	{ $$ = node2(N_GT, $1, $3); }
	| relational_expression LE_OP shift_expression	{ $$ = node2(N_LE, $1, $3); }
	| relational_expression GE_OP shift_expression	{ $$ = node2(N_GE, $1, $3); }
	| shift_expression
	;

shift_expression
	: shift_expression LEFT_OP additive_expression	{ $$ = node2(N_LEFT, $1, $3); }
	| shift_expression RIGHT_OP additive_expression	{ $$ = node2(N_RIGHT, $1, $3); }
	| additive_expression
	;

additive_expression
	: additive_expression '+' multiplicative_expression	{ $$ = node2(N_ADD, $1, $3); }
	| additive_expression '-' multiplicative_expression	{ $$ = node2(N_SUB, $1, $3); }
	| multiplicative_expression
	;

multiplicative_expression
	: multiplicative_expression '*' unary_expression	{ $$ = node2(N_MUL, $1, $3); }
	| multiplicative_expression '/' unary_expression	{ $$ = node2(N_DIV, $1, $3); }
	| multiplicative_expression '%' unary_expression	{ $$ = node2(N_MOD, $1, $3); }
	| unary_expression
	;

unary_expression
	: unary_operator unary_expression	{ $$ = node1($1, $2); }
	| INC_OP unary_expression		{ $$ = node1(N_PREINC, $2); }
	| DEC_OP unary_expression		{ $$ = node1(N_PREDEC, $2); }
	| postfix_expression
	;

unary_operator
	: '*'	{ $$ = N_INDIR; }
	| '&'	{ $$ = N_ADDR; }
	| '-'	{ $$ = N_NEG; }
	| '!'	{ $$ = N_NOT; }
	;

postfix_expression
	: postfix_expression '[' expression ']'			{ $$ = node2(N_INDEX, $1, $3); }
	| postfix_expression '(' ')'				{ $$ = node1(N_CALL, $1); }
	| postfix_expression '(' argument_expression_list ')'	{ $$ = node2(N_CALL, $1, $3); }
	| postfix_expression INC_OP				{ $$ = node1(N_POSTINC, $1); }
	| postfix_expression DEC_OP				{ $$ = node1(N_POSTDEC, $1); }
	| primary_expression
	;

argument_expression_list
	: argument_expression_list ',' assignment_expression	{ $$ = node2(N_DUMMY, $3, $1); }
	| assignment_expression
	;

primary_expression
	: NAME			{ $$ = leaf1(N_NAME, $1); }
	| constant
	| '(' expression ')'	{ $$ = $2; }
	;

constant
	: LITERAL		{ $$ = leaf1(N_CONST, $1); }
	| STRING_LITERAL	{ $$ = leaf1(N_CONST, $1); }
	;

%%

int main(void)
{
	return yyparse();
}
