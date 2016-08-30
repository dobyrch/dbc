YFLAGS = -d
CFLAGS = $$(llvm-config --cflags)
LDFLAGS = $$(llvm-config --ldflags --libs --system-libs)

dbc: arch.o parse.o lex.o astnode.o codegen.o
	$(CXX) -o $@ $^ $(LDFLAGS)

libb.a: libb.c clashes
	$(CC) -nostdlib -c libb.c
	objcopy --redefine-syms=clashes libb.o
	ar rcs $@ libb.o

.PHONY: test
test: sample.b libb.a dbc
	./dbrc $<
	./a.out
