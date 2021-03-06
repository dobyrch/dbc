Doug's B Compiler
=================

**dbc** aims to replicate of Ken Thompson's implementation of [B](https://en.wikipedia.org/wiki/B_(programming_language)) for the PDP-11 (see [here](https://www.bell-labs.com/usr/dmr/www/kbman.pdf)), with a few differences:

- Instead of constructing a "reverse Polish threaded code interpreter" as described in section 12.0 of the manual, `dbc` generates [LLVM](https://en.wikipedia.org/wiki/LLVM) bitcode. This is for two reasons: First, so I could get my feet wet with LLVM; second, so `dbc` can leverage the existing optimizers for LLVM.

- `dbc` targets x86 and x86-64, so values may be larger than on the 16-bit PDP-11.  This also means character literals may contain as many as 8 characters instead of 2.

What works and what doesn't
---------------------------

This project is still rough around the edges, but it should be functional enough to play around with the B language.  The example program from section 9.2, which calculates 4000 digits of _e_, compiles and runs flawlessly (well, it does for me). Run `make test` to try it out.

- All operators and statements are implemented, although they haven't all been thoroughly tested.  I suspect some edge cases with oddly placed labels may generate invalid LLVM IR.

- Every library function has been implemented, although `gtty()` and `stty()` differ slightly from their descriptions in the manual: they both require a *4*-word vector as a second argument.  They are equivalent to `tcgetattr` and `tcsetattr`, respectively, but use a vector instead of `struct termios` to hold the four flag fields.

- The error diagnostics still need some work—several different errors may be printed for the same line, and the line number is not always correct.

- Indirectly assigning to a global variable will have strange results (e.g. `foo = 40` is fine, but `*&foo = 40` will actually set foo to 5, not 40).  This issue does not affect local variable assignment, nor does it affect assignment to array indices (i.e. if `foo` is a global vector, `foo[3] = 40` works as expected). The problem stems from a kludge which is necessary to achieve correct pointer arithmetic semantics.

- A simple definition may contain at most one value in its initializer list.  I have not yet found a reasonable way to implement the semantics described in section 7.1 of the manual; use a vector definition instead (e.g. `foo[5] 1, 2, 3, 4, 5;` instead of `foo 1 2 3 4 5;`).  Incidentally, this same restriction seemed to be present in the H6070 implementation of B.

- Global initializer lists may not contain strings or names of other globals (yet).

How to run
----------

You'll need a few things to get started:

- A computer running Linux or FreeBSD¹ (x86 or x86-64)²
- LLVM libraries and header files
- Your favorite C compiler (tested with clang 3.8 and gcc 6.1)
- A C++ compiler of your choice (for linking with LLVM; clang++ and g++ should both work)
- yacc (tested with bison and byacc)
- lex (tested with flex)
- GNU binutils (ld, ar, as, and objcopy)
- GNU make
- This source code

Check your distribution's repositories for any missing components (Ubuntu, for example, also requires the `zlib1g-dev` package)

Once you have everything installed, `cd` to the directory containing the source code for `dbc` and run `make`. If all goes well, you should now be the proud owner of a binary named `dbc`. While you're at it, also run `make libb.a` to generate the B standard library.

Now that you've compiled the compiler, you're just a few steps away from compiling your first B program.  The observant reader will notice that these steps closely resemble the procedure from section 10.0 of the B manual:

1. `./dbc sample.b sample.bc` - Compile your B source code into an intermediate language (LLVM bitcode)
2. `llc sample.bc` - Convert the bitcode into assembly language
3. `as sample.s -o sample.o` - Compile the assembly source into an object file
4. `ld sample.o libb.a` - Link your object file against the B library
5. `./a.out` - Run your first B program!

To make the process less tedious, the `dbrc` script may be used to execute the same sequence of commands: `./dbrc sample.b` (Don't expect anything fancy—it runs that exact sequence of commands, nothing more, nothing less).

¹FreeBSD and Linux use [different conventions](https://www.freebsd.org/doc/en/books/developers-handbook/x86-system-calls.html) for making syscalls on x86; run `brandelf -t Linux a.out` before executing a binary compiled by `dbrc` on FreeBSD (note that this is only necessary for x86, not x86_64)

²ARM is also supported, but compiled programs must be linked against libc due to the lack of an integer division instruction on ARM

Differences between B and C
---------------------------

- Reversed assignment operators: B uses `x =+ 2`, not `x += 2` (although the former style _was_ legal in older C programs, according to K&R)
- B also has assignment operators for comparisons: `x =< 2` stores the result of `x < 2` in x; `x === 2` (that's right, triple `=`) stores the result of `x == 2` in x
- The escape character is `*`, not `\`: `'*n'` is a newline character
- No types: all values are the same width
- As a consequence of the previous point, character literals may contain more than one character, so `putchar('ab')` is perfectly fine; interestingly, so-called "multichars" are also valid C, since character literals in C are actually integers (although `putchar` on my system only prints the last character)
- Variables *must* be declared with either `auto` or `extrn`, unlike in C where the storage class specifier is usually implicit (since B doesn't have types, those keywords are necessary to indicate a declaration)
- No floating point numbers
- No structs or unions
- The PDP-11 implementation of B lacks the keywords `for`, `do`, `break`, `continue`, and `default`, as well as the `~` and `^` operators, although they do make an appearance in the Honeywell 6070 version
- No `&&` and `||` operators, only bitwise `&` and `|`; the H6070 documentation mentions that `&` and `|` were overloaded to perform logical operations when used inside of `if`, `while`, or a conditional expression, but the PDP-11 manual does not assign any additional behavior to the bitwise operators
- Function definitions don't require braces for a single statement: `main() return(0);` is a valid B program
- A `return` statement requires parentheses: `return(x)`
- ...but `switch` does not: `switch x { /* do stuff */ }`
- `argv` is a global variable, not an argument to `main()`; `argv[0]` contains the number of command line arguments, not the program name

Helpful references
------------------

- [Users' Reference to B](https://www.bell-labs.com/usr/dmr/www/kbman.html) - An HTML version of the B Manual for PDP-11, transcribed and edited by none other than Dennis Ritchie
- [The Programming Language B](https://www.bell-labs.com/usr/dmr/www/bintro.html) - Additional B references written by Steve Johnson and Brian Kernighan; they describe a later version of B for the Honeywell 6070 (also noteworthy: Kernighan's tutorial contains what may be the first "hello world")
- [B man page](http://minnie.tuhs.org/cgi-bin/utree.pl?file=V1/man/man1/b.1) - The `man` page for B from Version 1 Unix

- [LLVM Tutorial](http://llvm.org/docs/tutorial/index.html) - A handy guide for building your own compiler for a simple programming language
- [LLVM Language Reference](http://llvm.org/docs/LangRef.html)
- [LLVM-C API](http://www.llvm.org/docs/doxygen/html/group__LLVMC.html) - These just look like auto-generated docs, so it may be more convenient to read the headers in `/usr/include/llvm-c`

- [ANSI C Yacc grammar](http://www.quut.com/c/ANSI-C-grammar-y.html)
- [GNU Bison Manual](https://www.gnu.org/software/bison//manual/)
- [Flex Manual](http://flex.sourceforge.net/manual/)
