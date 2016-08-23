%{
#include <string.h>
#include "dbc.tab.h"

static void strip_comment(void);
%}

%option yylineno
%option noyywrap
%option nounput

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]

%%

"/*"			{ strip_comment(); }

"auto"			{ return(AUTO); }
"case"			{ return(CASE); }
"else"			{ return(ELSE); }
"extrn"			{ return(EXTRN); }
"goto"			{ return(GOTO); }
"if"			{ return(IF); }
"return"		{ return(RETURN); }
"switch"		{ return(SWITCH); }
"while"			{ return(WHILE); }

{L}({L}|{D})*		{ yylval.val = strdup(yytext); return(NAME); }

0{D}+			{ yylval.val = strdup(yytext); return(LITERAL); }
{D}+			{ yylval.val = strdup(yytext); return(LITERAL); }
'(\*.|[^\*'])+'		{ yylval.val = strdup(yytext); return(LITERAL); }

\"(\*.|[^\*"])*\"	{ yylval.val = strdup(yytext); return(LITERAL); }

"=>>"			{ return(RIGHT_ASSIGN); }
"=<<"			{ return(LEFT_ASSIGN); }
"=+"			{ return(ADD_ASSIGN); }
"=-"			{ return(SUB_ASSIGN); }
"=*"			{ return(MUL_ASSIGN); }
"=/"			{ return(DIV_ASSIGN); }
"=%"			{ return(MOD_ASSIGN); }
"=&"			{ return(AND_ASSIGN); }
"=|"			{ return(OR_ASSIGN); }
"==="			{ return(EQ_ASSIGN); }
"=!="			{ return(NE_ASSIGN); }
"=<="			{ return(LE_ASSIGN); }
"=<"			{ return(LT_ASSIGN); }
"=>="			{ return(GE_ASSIGN); }
"=>"			{ return(GT_ASSIGN); }
">>"			{ return(RIGHT_OP); }
"<<"			{ return(LEFT_OP); }
"++"			{ return(INC_OP); }
"--"			{ return(DEC_OP); }
"<="			{ return(LE_OP); }
">="			{ return(GE_OP); }
"=="			{ return(EQ_OP); }
"!="			{ return(NE_OP); }
"("			{ return('('); }
")"			{ return(')'); }
"["			{ return('['); }
"]"			{ return(']'); }
"{"			{ return('{'); }
"}"			{ return('}'); }
","			{ return(','); }
";"			{ return(';'); }
":"			{ return(':'); }
"!"			{ return('!'); }
"*"			{ return('*'); }
"/"			{ return('/'); }
"%"			{ return('%'); }
"+"			{ return('+'); }
"-"			{ return('-'); }
"<"			{ return('<'); }
">"			{ return('>'); }
"&"			{ return('&'); }
"|"			{ return('|'); }
"?"			{ return('?'); }
"="			{ return('='); }

[ \t\v\n\f]		{ /* Ignore whitespace */ }
.			{ return(JUNK); }

%%

static void strip_comment(void)
{
	int line;
	char c1, c2;

	line = yylineno;
	c1 = input();

	do {
		while ((c2 = input()) != '/') {
			if (!c1 && !c2) {
				printf("%d: */\n", line);
				exit(EXIT_FAILURE);
			}

			c1 = c2;
		}
	} while (c1 != '*');
}
