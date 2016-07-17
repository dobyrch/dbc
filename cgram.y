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

ival_list
	: ival
	| ival_list ',' ival
	;

ival
	: constant
	| NAME
	;

identifier_list
	: NAME
	| identifier_list ',' NAME
	;

statement
	: AUTO init_list ';'
	| EXTRN name_list ';'
	| NAME ':' statement
	| CASE constant ':' statement

	| '{' '}'
	| '{' statement_list '}'

	| IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement

	| WHILE '(' expression ')' statement
	| SWITCH '(' expression ')' statement
	| GOTO NAME ';'

	| RETURN ';'
	| RETURN expression ';'

	| expression ';'
	| ';'
	;

init_list
	: init
	| init_list ',' init
	;

init
	: NAME
	| NAME constant
	;

name_list
	: NAME
	| name_list ',' NAME
	;

statement_list
	: statement
	| statement_list statement
	;

expression
	: expression ',' assignment_expression
	| assignment_expression
	;

assignment_expression
	: unary_expression assignment_operator assignment_expression
	| conditional_expression
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

conditional_expression
	: inclusive_or_expression '?' expression ':' conditional_expression
	| inclusive_or_expression
	;

inclusive_or_expression
	: inclusive_or_expression '|' exclusive_or_expression
	| exclusive_or_expression
	;

exclusive_or_expression
	: exclusive_or_expression '^' and_expression
	| and_expression
	;

and_expression
	: and_expression '&' equality_expression
	| equality_expression
	;

equality_expression
	: equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	| relational_expression
	;

relational_expression
	: relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression LE_OP shift_expression
	| relational_expression GE_OP shift_expression
	| shift_expression
	;

shift_expression
	: shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	| additive_expression
	;

additive_expression
	: additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
	| multiplicative_expression
	;

multiplicative_expression
	: multiplicative_expression '*' unary_expression
	| multiplicative_expression '/' unary_expression
	| multiplicative_expression '%' unary_expression
	| unary_expression
	;

unary_expression
	: unary_operator unary_expression
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| postfix_expression
	;

unary_operator
	: '*'
	| '&'
	| '-'
	| '!'
	;

postfix_expression
	: postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	| primary_expression
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

primary_expression
	: NAME
	| constant
	| '(' expression ')'
	;

constant
	: LITERAL
	| STRING_LITERAL
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
