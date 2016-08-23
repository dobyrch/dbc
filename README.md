Doug's B Compiler
=================

**dbc** aims to replicate of Ken Thompson's implementation of [B](https://en.wikipedia.org/wiki/B_(programming_language)) for the PDP-11 (see [here](https://www.bell-labs.com/usr/dmr/www/kbman.pdf)), with a few differences:

- Instead of constructing a "reverse Polish threaded code interpreter" as described in section 12.0 of the manual, `dbc` generates [LLVM](https://en.wikipedia.org/wiki/LLVM) bitcode. This is for two reasons: First, so I could get my feet wet with LLVM; second, so `dbc` can leverage the existing optimizers for LLVM.

- `dbc` targets x86-64, so all values are 64 bits wide (unlike the implementation for PDP-11, which was a 16-bit architecture).  Practically speaking, this means that character literals may contain up to 8 characters instead of 2, and integers can be a _lot_ bigger.

What works and what doesn't
---------------------------

This project is still awfully rough around the edges, but it should be functional enough to play around with the B language.  The example program from section 9.2, which calculates 4000 digits of _e_, compiles and runs flawlessly (well, it does for me). Run `make test` to try it out.

- All operators and statements are implemented, although they haven't all been thoroughly tested.  I suspect some edge cases with oddly placed labels may generate invalid LLVM IR.

- Every library function has been implemented, except for `gtty()` and `stty()`.

- The error diagnostics still need some work—several different errors may be printed for the same line, and the line number is not always correct.

- Indirectly assigning to a global variable will have strange results (e.g. `foo = 40` is fine, but `*&foo = 40` will actually set foo to 5, not 40).  This issue does not affect local variable assignment, nor does it affect assignment to array indices (i.e. if `foo` is a global vector, `foo[3] = 40` works as expected). The problem stems from a kludge which is necessary to achieve correct pointer arithmetic semantics.

- Global vector initializer lists may not contain strings.  I suspect this may be a bug in LLVM, (see [`llvm_bug.c`](https://github.com/dobyrch/dbc/blob/master/llvm_bug.c), although I haven't looked into it further.

- A simple definition may contain at most one value in its initializer list.  I have yet not found a reasonable way to implement the semantics described in section 7.1 of the manual; use a vector definition instead (e.g. `foo[5] 1, 2, 3, 4, 5;` instead of `foo 1 2 3 4 5;`).  Incidentally, this same restriction seemed to be present in the H6070 implementation of B.

Differences Between B and C
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
