CC = clang
CPP = clang++
YFLAGS = -d
CFLAGS = $$(llvm-config --cflags)
LDFLAGS = $$(llvm-config --ldflags --libs --system-libs)

dbc: parse.o lex.o astnode.o codegen.o
	$(CPP) -o $@ $^ $(LDFLAGS)

libb.a: dbc libb.b libb.c clashes
	./dbc < libb.b
	mv dbc.bc libb.bc
	$(CC) -nostdlib -o blibb.o -c libb.bc
	$(CC) -nostdlib -o clibb.o -c libb.c
	objcopy --redefine-syms=clashes clibb.o
	ar rcs libb.a blibb.o clibb.o

.PHONY: test
test: dbc libb.a sample.b
	./dbc < ./sample.b
	mv dbc.bc sample.bc
	$(CC) -nostdlib sample.bc libb.a
	./a.out
