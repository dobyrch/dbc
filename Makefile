CC = clang
CPP = clang++
YFLAGS = -d
CFLAGS = $$(llvm-config --cflags)
LDFLAGS = $$(llvm-config --ldflags --libs --system-libs)

dbc: parse.o lex.o astnode.o codegen.o
	$(CPP) -o $@ $^ $(LDFLAGS)

%.bc: %.b dbc
	./dbc $< $@

libb.a: libb.bc libb.c clashes
	$(CC) -nostdlib -o blibb.o -c libb.bc
	$(CC) -nostdlib -o clibb.o -c libb.c
	objcopy --redefine-syms=clashes clibb.o
	ar rcs $@ blibb.o clibb.o

.PHONY: test
test: sample.bc libb.a
	$(CC) -nostdlib $^
	./a.out
