YFLAGS = -d
CFLAGS = $$(llvm-config --cflags)
LDFLAGS = $$(llvm-config --ldflags --libs --system-libs)

dbc: arch.o parse.o lex.o astnode.o codegen.o
	$(CXX) -o $@ $^ $(LDFLAGS)

libb.a: libb.c start.S clashes
	$(CC) -c libb.c start.S
	objcopy libb.o --redefine-syms=clashes
	ar rcs $@ libb.o start.o

.PHONY: test
test: sample.b libb.a dbc
	./dbrc $<
	./a.out
