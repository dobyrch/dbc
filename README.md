Doug's B Compiler
=================

**dbc** aims to replicate of Ken Thompson's implementation of B for the PDP-11 (see [here](https://www.bell-labs.com/usr/dmr/www/kbman.pdf)), with a few differences.

- Instead of constructing a "reverse Polish threaded code interpreter" as described in section 12.0 of the manual, **dbc** generates LLVM bitcode. This is for two reasons: First, so I could get my feet wet with LLVM; second, so **dbc** can leverage the existing optimizers for LLVM.

- **dbc** targets x86-64, so all values are 64 bits wide (unlike the PDP-11 implementation, which was a 16-bit architecture).  Practically speaking, this means that character literals may contain up to 8 characters instead of 2, and integers can be a _lot_ bigger.

- The compiler diagnostics aren't quite as terse as those described in section 14.0, although I would like to implement a "legacy mode" that uses the original error codes.
