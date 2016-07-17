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
	| labeled_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	| expression_statement
	;

automatic_declaration
	: AUTO name_init_list ';'

name_init_list
	: name_init
	| name_init_list ',' name_init
	;

external_declaration
	: EXTRN name_list ';'
	;

name_list
	: NAME
	| name_list ',' NAME

name_init
	: NAME
	| NAME constant
	;


labeled_statement
	: NAME ':' statement
	| CASE constant ':' statement
	;

compound_statement
	: '{' '}'
	| '{' statement_list '}'
	;

selection_statement
	: IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement
	| SWITCH '(' expression ')' statement
	;

expression_statement
	: ';'
	| expression ';'
	;

iteration_statement
	: WHILE '(' expression ')' statement
	;

jump_statement
	: GOTO NAME ';'
	| RETURN ';'
	| RETURN expression ';'
	;

constant
	: LITERAL
	| STRING_LITERAL
	;

ival_list
	: ival
	| ival_list ',' ival
	;

primary_expression
	: NAME
	| LITERAL
	| STRING_LITERAL
	| '(' expression ')'
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression '.' NAME
	| postfix_expression PTR_OP NAME
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| unary_operator unary_expression
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression
	| multiplicative_expression '/' unary_expression
	| multiplicative_expression '%' unary_expression
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
	;

shift_expression
	: additive_expression
	| shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression LE_OP shift_expression
	| relational_expression GE_OP shift_expression
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression AND_OP inclusive_or_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
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

expression
	: assignment_expression
	| expression ',' assignment_expression
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
