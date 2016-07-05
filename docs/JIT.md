= A Machine Code JIT Compiler for Curv

== Which JIT library to use?
LLVM is the dominant JIT library. But it has a serious problem:
compilation is too slow for interactive use. Also, there are
bugs and limitations that aren't immediately apparent. LLVM isn't
really that good for JIT, which creates a chicken-and-egg problem:
most people don't use it, so the problems get fixed very slowly.

Most serious language projects use a custom machine code JIT compiler.
In the Python world, the Unladen Swallow project (using LLVM) failed,
and was superceded by PyPy, which uses a custom machine code generator.

There is perhaps an open niche for a really good JIT library,
and the mythology around LLVM perhaps prevents a serious effort at
creating this library (because: why not just improve LLVM instead?)
The answer may be that the LLVM architecture is incompatible with
fast, interactive-grade compilation.

LLVM is the dominant standard, used by Haskell, Swift, Erlang, others.
Unequaled power and flexibility.
Fantastically complex, documentation difficult to navigate.
Memory management is problematic:
* All allocated objects are owned by an LLVMContext instance?
* In practice, I need a single global LLVMContext shared by all scripts
  compiled by a Curv session, and this thing will just keep growing.
Two JIT interfaces:
* Legacy JIT interface used by the tutorial. Custom instruction encoder.
  Allegedly not supported by LLVM 3.7.1, not sure when it went away.
  * LLVM 3.4: support for exception handling has been removed from old JIT.
    Use MCJIT for EH support.
* Newer MCJIT interface. Uses MC to encode instructions. Needs to perform
  runtime linking of object files (it emits ELF object files into memory).
  Less JITy (once a module is compiled, it's done, you can't update it further).
  * oct2014: mcjit doesn't support exception catching on windows, that would
    require COFF support.
  * The current design for "stack trace inside of an exception" relies on
    low cost exception catching (which is more expensive on windows).
    Instead of throwing an exception, I could return an exception value,
    and test for a tag value in the Value or pointer returned by a function.
  * IR to machine code in MCJIT is slow (implied by forum post).
    Open Shading Language still uses LLVM 3.4, because 50% slower JIT time is
    a big deal for human interaction.

LibLightning -- http://www.gnu.org/software/lightning/
Very simple and fast JIT library.
However, I don't see any way to call C functions from JIT generated code.
Let alone catch exceptions raised by C functions.

LibGccJit --
Project to refactor GCC code into a backend JIT library.
In gcc trunk as of gcc 5. Alpha quality code with unstable API.
It uses context objects, but there's documentation, so the lifetimes
of objects are clear. The generated machine code is independent of the
context object, which can be freed before running the machine code.
May 2016: author has just learned about need for tail call support
(LLVM had this years ago, albeit intel only).

LibJit -- gnu.org/software/libjit
Created in 2004, barely updated or used any more.

== How To Use LLVM?

Where are the LLVM module boundaries in Curv?
Let's assume no optimization across module boundaries.
* each expression/definition typed interactively is a module
* each script file is a module

So we compile a command line expression/script file to an LLVM module,
then evaluate it.

== Optimized Machine Code
If I were compiling Curv to machine code,
I'd use some optimizations that aren't really practical in the byte code VM.

* **Lazy Boxing.**
  Keep values their unboxed representation where possible.
  Convert to boxed representation only when necessary.
  In an Expression node, I would keep track of the compile time type of
  the expression as a bitmask of the 8 basic types.
* **Lazy Ownership.**
  I would be lazy about incrementing reference counts.
  If I am copying a curv::Value from another storage location,
  I don't automatically increment the reference count.
  Instead, I just mark the Expression node as "borrowed";
  "owned" would mean we own the reference.
  * The function calling convention assumes that reference parameters
    are borrowed. The caller is responsible for releasing argument references,
    as necessary. This is no good for tail call optimization.
    So, we only optimize *recursive* tail calls. A self-recursive tail call
    turns into a branch to the beginning of the function, after updating
    the parameter values. A mutually recursive tail call to another function
    G involves inline-expanding the body of G into the current function.
    There's a top level loop that selects which function to enter on each
    iteration. This approach to tail-call optimization has other benefits:
    * It's compatible with the C calling convention, which may be good for
      interoperability with C.
    * LLVM doesn't support tail-recursion optimization on ARM.

These optimizations make the code run faster, but they make introspection
of the VM state more difficult (for debugging and stack unwinding).
So you need a map from instruction positions to stack & register state.
This map is expensive to construct, which slows down the compile enough
to be undesirable for interactive use. So, the VM needs to provide a choice
between fast compile and "adequate" run time performance (10-20x slower than C),
vs slow compile and fast run time. There should be an ahead-of-time compile
tool that compiles a top-level script into a shared object/DLL.
