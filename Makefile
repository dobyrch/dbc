dbc: dbc.y dbc.l astnode.c astnode.h codegen.c codegen.h constants.h
	bison -v -x -d -t dbc.y
	flex dbc.l
	clang -c astnode.c codegen.c dbc.tab.c lex.yy.c $$(llvm-config --cflags)
	clang++ -o dbc astnode.o codegen.o dbc.tab.o lex.yy.o $$(llvm-config --ldflags --libs --system-libs)
	xsltproc /usr/share/bison/xslt/xml2xhtml.xsl dbc.xml > dbc.html

test: dbc libb.c
	./dbc < ./sample.b
	mv dbc.bc sample.bc
	./dbc < ./libb.b
	mv dbc.bc libb.bc
	clang -Wl,--defsym=char=char_ -nostdlib -Wno-main sample.bc libb.bc libb.c
	./a.out
