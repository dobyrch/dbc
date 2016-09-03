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

%{
#include <string.h>
#include "y.tab.h"

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
