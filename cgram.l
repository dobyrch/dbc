D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]

%{
#include "types.h"
#include "cgram.tab.h"

void count(void);
void comment(void);
char *strdup(const char *);
%}

%option noyywrap

%%

"/*"			{ comment(); }

"auto"			{ count(); return(AUTO); }
"case"			{ count(); return(CASE); }
"else"			{ count(); return(ELSE); }
"extrn"			{ count(); return(EXTRN); }
"goto"			{ count(); return(GOTO); }
"if"			{ count(); return(IF); }
"return"		{ count(); return(RETURN); }
"switch"		{ count(); return(SWITCH); }
"while"			{ count(); return(WHILE); }

{L}({L}|{D})*		{ count(); yylval.val = strdup(yytext); return(NAME); }

0{D}			{ count(); yylval.val = strdup(yytext); return(LITERAL); }
{D}+			{ count(); yylval.val = strdup(yytext); return(LITERAL); }
'(\*.|[^\*'])+'		{ count(); yylval.val = strdup(yytext); return(LITERAL); }

\"(\*.|[^\*"])*\"	{ count(); yylval.val = strdup(yytext); return(STRING_LITERAL); }

"=>>"			{ count(); return(RIGHT_ASSIGN); }
"=<<"			{ count(); return(LEFT_ASSIGN); }
"=+"			{ count(); return(ADD_ASSIGN); }
"=-"			{ count(); return(SUB_ASSIGN); }
"=*"			{ count(); return(MUL_ASSIGN); }
"=/"			{ count(); return(DIV_ASSIGN); }
"=%"			{ count(); return(MOD_ASSIGN); }
"=&"			{ count(); return(AND_ASSIGN); }
"=^"			{ count(); return(XOR_ASSIGN); }
"=|"			{ count(); return(OR_ASSIGN); }
"==="			{ count(); return(EQ_ASSIGN); }
"=!="			{ count(); return(NE_ASSIGN); }
">>"			{ count(); return(RIGHT_OP); }
"<<"			{ count(); return(LEFT_OP); }
"++"			{ count(); return(INC_OP); }
"--"			{ count(); return(DEC_OP); }
"<="			{ count(); return(LE_OP); }
">="			{ count(); return(GE_OP); }
"=="			{ count(); return(EQ_OP); }
"!="			{ count(); return(NE_OP); }
";"			{ count(); return(';'); }
"{"			{ count(); return('{'); }
"}"			{ count(); return('}'); }
","			{ count(); return(','); }
":"			{ count(); return(':'); }
"="			{ count(); return('='); }
"("			{ count(); return('('); }
")"			{ count(); return(')'); }
"["			{ count(); return('['); }
"]"			{ count(); return(']'); }
"&"			{ count(); return('&'); }
"^"			{ count(); return('^'); }
"!"			{ count(); return('!'); }
"-"			{ count(); return('-'); }
"+"			{ count(); return('+'); }
"*"			{ count(); return('*'); }
"/"			{ count(); return('/'); }
"%"			{ count(); return('%'); }
"<"			{ count(); return('<'); }
">"			{ count(); return('>'); }
"|"			{ count(); return('|'); }
"?"			{ count(); return('?'); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%


void comment(void)
{
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
		putchar(c);

	if ((c1 = input()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}

	if (c != 0)
		putchar(c1);

	ECHO;
}


int column = 0;

void count(void)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}

char *strdup(const char *s)
{
	/* TODO: Error handling */
	char *s_new = malloc(strlen(s) + 1);
	strcpy(s_new, s);

	return s_new;
}
