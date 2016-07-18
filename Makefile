cgram: cgram.y cgram.l external.c  external.h types.h
	bison -v -x -d -t cgram.y
	flex cgram.l
	clang -ansi -O0 -g -o cgram external.c cgram.tab.c lex.yy.c
	xsltproc /usr/share/bison/xslt/xml2xhtml.xsl cgram.xml > cgram.html

test: cgram
	./cgram < ./sample.b
