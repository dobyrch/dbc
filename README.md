Doug's B Compiler
=================

**dbc** aims to replicate of Ken Thompson's implementation of [B](https://en.wikipedia.org/wiki/B_(programming_language)) for the PDP-11 (see [here](https://www.bell-labs.com/usr/dmr/www/kbman.pdf)), with a few differences.

- Instead of constructing a "reverse Polish threaded code interpreter" as described in section 12.0 of the manual, **dbc** generates LLVM bitcode. This is for two reasons: First, so I could get my feet wet with LLVM; second, so **dbc** can leverage the existing optimizers for LLVM.

- **dbc** targets x86-64, so all values are 64 bits wide (unlike the PDP-11 implementation, which was a 16-bit architecture).  Practically speaking, this means that character literals may contain up to 8 characters instead of 2, and integers can be a _lot_ bigger.

- The compiler diagnostics aren't quite as terse as those described in section 14.0, although I would like to implement a "legacy mode" that uses the original error codes.

What works and what doesn't
---------------------------

This project is still awfully rough around the edges, but it should be functional enough to play around with the B language.  The example program from 9.2, which calculates 4000 digits of _e_, compiles and runs flawlessly (well, it does for me). Run `make test` to try it out.

All operators and statements are implemented, although they haven't all been thoroughly tested.  I suspect some edge cases with oddly placed labels may generate invalid LLVM IR.

Only one function may be defined currently (and if you expect that function to do anything, you should probably call it `main` so the linker can find it).

String literals were only added recently and suffer from some endianess issues, so don't expect characters to be in the right order when you print them.

The library functions in section 8.0 have not been implemented, although clang will happily link your B program against libc instead.  Some functions, like `putchar` and `printf`, are still more or less functional if you know what you're doing.

Differences Between B and C
---------------------------

- Reversed assignment operators: B uses `x =+ 2`, not `x += 2`
- B also has assignment operators for comparisons: `x =< 2` stores the result of `x < 2` in x; `x === 2` (that's right, triple `=`) stores the result of `x == 2` in x
- The escape character is `*`, not `\`: `'*n'` is a newline character
- Function definitions don't require braces for a single statement: `main() printf("hello world");` is a valid B program
- No types: All values are the same width
- As a consequence of the previous point, character literals may contain more than one character, so `putchar('ab')` is perfectly fine; interestingly, so-called "multichars" are also valid C, since character literals in C are actually integers (although `putchar` on my system only prints the last character)
- Variables *must* be declared with either `auto` or `extrn`, unlike in C where the storage class specifier is usually implicit (Since B doesn't have types, those keywords are necessary to indicate a declaration)
- No floating point numbers
- No structs or unions
- The PDP-11 implementation of B lacks the keywords `for`, `do`, `break`, `continue`, and `default`, as well as the `~` and `^` operators, although they do make an appearance in the Honeywell 6070 version
- A `return` statement requires parentheses: `return(x)`
- ...but `switch` does not: `switch x { /* do stuff */ }`
- `argv` is a global variable, not an argument to `main()`; `argv[0]` contains the number of command line arguments, not the program name



Helpful references
------------------

- [Users' Reference to B](https://www.bell-labs.com/usr/dmr/www/kbman.html) - An HTML version of the B Manual for PDP-11, transcribed and edited by none other than Dennis Ritchie
- [The Programming Language B](https://www.bell-labs.com/usr/dmr/www/bintro.html) - Additional B references written by Steve Johnson and Brian Kernighan; they describe a later version of B for the Honeywell 6070 (also noteworthy: Kernighan's tutorial contains what may be the first "hello world")

- [LLVM Tutorial](http://llvm.org/docs/tutorial/index.html) - A handy guide for building your own compiler for a simple programming language
- [LLVM Language Reference](http://llvm.org/docs/LangRef.html)
- [LLVM-C API](http://www.llvm.org/docs/doxygen/html/group__LLVMC.html) - These just look like auto-generated docs, so it may be more convenient to read the headers in `/usr/include/llvm-c`

- [GNU Bison Manual](https://www.gnu.org/software/bison//manual/)
- [Flex Manual](http://flex.sourceforge.net/manual/)
