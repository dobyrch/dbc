cgram: cgram.y cgram.l
	bison -v -x -d -t cgram.y
	flex cgram.l
	clang -ansi -O0 -g -o cgram cgram.tab.c lex.yy.c
	xsltproc /usr/share/bison/xslt/xml2xhtml.xsl cgram.xml > cgram.html

test: cgram
	./cgram < ./sample.b
