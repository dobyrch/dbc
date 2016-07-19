cgram: cgram.y cgram.l external.c  external.h types.h
	bison -v -x -d -t cgram.y
	flex cgram.l
	clang  `llvm-config --cflags` -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -O0 -g -c external.c cgram.tab.c lex.yy.c
	clang++ `llvm-config --cxxflags --ldflags` external.o cgram.tab.o lex.yy.o `llvm-config --libs --system-libs` -o cgram
	xsltproc /usr/share/bison/xslt/xml2xhtml.xsl cgram.xml > cgram.html

test: cgram
	./cgram < ./sample.b
