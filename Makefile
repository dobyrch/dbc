dbc: dbc.y dbc.l astnode.c astnode.h codegen.c  codegen.h
	bison -v -x -d -t dbc.y
	flex dbc.l
	clang `llvm-config --cflags` -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -O0 -g -c astnode.c codegen.c dbc.tab.c lex.yy.c
	clang++ `llvm-config --cxxflags --ldflags` astnode.o codegen.o dbc.tab.o lex.yy.o `llvm-config --libs --system-libs` -o dbc
	xsltproc /usr/share/bison/xslt/xml2xhtml.xsl dbc.xml > dbc.html

test: dbc libb.c
	./dbc < ./sample.b
	clang dbc.bc
	./a.out
