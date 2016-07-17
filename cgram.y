%token NAME LITERAL STRING_LITERAL
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN EQ_ASSIGN NE_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

%token AUTO EXTRN

%token CASE IF ELSE SWITCH WHILE GOTO RETURN

%%

program
	: /* empty */
	| definitions
	;

definitions
	: definition
	| definitions definition
	;

definition
	: simple_definition
	| vector_definition
	| function_definition
	;

simple_definition
	: NAME ';'
	| NAME  ival_list ';'
	;

vector_definition
	: NAME '[' ']' ';'
	| NAME '[' ']' ival_list ';'
	| NAME '[' constant ']' ';'
	| NAME '[' constant ']' ival_list ';'
	;

function_definition
	: NAME '(' ')' statement
	| NAME '(' identifier_list ')' statement
	;

ival
	: constant
	| NAME
	;

statement
	: automatic_declaration
	| external_declaration
	| label_statement
	| case_statement
	| compound_statement
	| conditional_statement
	| while_statement
	| switch_statement
	| goto_statement
	| return_statement
	| rvalue_statement
	| null_statement
	;

automatic_declaration
	: AUTO init_list ';'

init_list
	: init
	| init_list ',' init
	;

init
	: NAME
	| NAME constant
	;

external_declaration
	: EXTRN name_list ';'
	;

name_list
	: NAME
	| name_list ',' NAME
	;

label_statement
	: NAME ':' statement
	;

case_statement
	: CASE constant ':' statement
	;

compound_statement
	: '{' '}'
	| '{' statement_list '}'
	;

conditional_statement
	: IF '(' rvalue ')' statement
	| IF '(' rvalue ')' statement ELSE statement
	;

while_statement
	: WHILE '(' rvalue ')' statement
	;

switch_statement
	: SWITCH '(' rvalue ')' statement
	;

goto_statement
	: GOTO NAME ';'
	;

return_statement
	: RETURN ';'
	| RETURN rvalue ';'
	;

rvalue_statement
	: rvalue ';'
	;

null_statement
	: ';'
	;

constant
	: LITERAL
	| STRING_LITERAL
	;

ival_list
	: ival
	| ival_list ',' ival
	;

primary_rvalue
	: NAME
	| LITERAL
	| STRING_LITERAL
	| '(' rvalue ')'
	;

postfix_rvalue
	: primary_rvalue
	| postfix_rvalue '[' rvalue ']'
	| postfix_rvalue '(' ')'
	| postfix_rvalue '(' argument_rvalue_list ')'
	| postfix_rvalue '.' NAME
	| postfix_rvalue PTR_OP NAME
	| postfix_rvalue INC_OP
	| postfix_rvalue DEC_OP
	;

argument_rvalue_list
	: assignment_rvalue
	| argument_rvalue_list ',' assignment_rvalue
	;

unary_rvalue
	: postfix_rvalue
	| INC_OP unary_rvalue
	| DEC_OP unary_rvalue
	| unary_operator unary_rvalue
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

multiplicative_rvalue
	: unary_rvalue
	| multiplicative_rvalue '*' unary_rvalue
	| multiplicative_rvalue '/' unary_rvalue
	| multiplicative_rvalue '%' unary_rvalue
	;

additive_rvalue
	: multiplicative_rvalue
	| additive_rvalue '+' multiplicative_rvalue
	| additive_rvalue '-' multiplicative_rvalue
	;

shift_rvalue
	: additive_rvalue
	| shift_rvalue LEFT_OP additive_rvalue
	| shift_rvalue RIGHT_OP additive_rvalue
	;

relational_rvalue
	: shift_rvalue
	| relational_rvalue '<' shift_rvalue
	| relational_rvalue '>' shift_rvalue
	| relational_rvalue LE_OP shift_rvalue
	| relational_rvalue GE_OP shift_rvalue
	;

equality_rvalue
	: relational_rvalue
	| equality_rvalue EQ_OP relational_rvalue
	| equality_rvalue NE_OP relational_rvalue
	;

and_rvalue
	: equality_rvalue
	| and_rvalue '&' equality_rvalue
	;

exclusive_or_rvalue
	: and_rvalue
	| exclusive_or_rvalue '^' and_rvalue
	;

inclusive_or_rvalue
	: exclusive_or_rvalue
	| inclusive_or_rvalue '|' exclusive_or_rvalue
	;

logical_and_rvalue
	: inclusive_or_rvalue
	| logical_and_rvalue AND_OP inclusive_or_rvalue
	;

logical_or_rvalue
	: logical_and_rvalue
	| logical_or_rvalue OR_OP logical_and_rvalue
	;

conditional_rvalue
	: logical_or_rvalue
	| logical_or_rvalue '?' rvalue ':' conditional_rvalue
	;

assignment_rvalue
	: conditional_rvalue
	| unary_rvalue assignment_operator assignment_rvalue
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	| EQ_ASSIGN
	| NE_ASSIGN
	;

rvalue
	: assignment_rvalue
	| rvalue ',' assignment_rvalue
	;

identifier_list
	: NAME
	| identifier_list ',' NAME
	;

statement_list
	: statement
	| statement_list statement
	;

%%
#include <stdio.h>

extern char yytext[];
extern int column;

yyerror(s)
char *s;
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);

	return 0;
}


main()
{
	yyparse();
}
