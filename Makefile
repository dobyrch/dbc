CC = clang
CPP = clang++
YFLAGS = -d
CFLAGS = $$(llvm-config --cflags)
LDFLAGS = $$(llvm-config --ldflags --libs --system-libs)

dbc: parse.o lex.o astnode.o codegen.o
	$(CPP) -o $@ $^ $(LDFLAGS)

libb.a: libb.c clashes
	$(CC) -nostdlib -c libb.c
	objcopy --redefine-syms=clashes libb.o
	ar rcs $@ libb.o

%.bc: %.b dbc
	./dbc $< $@

.PHONY: test
test: sample.bc libb.a
	$(CC) -nostdlib $^
	./a.out
