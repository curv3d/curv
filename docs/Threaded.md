Consider LLVM for the VM.
* Compilation is slow.
* Execution is potentially fast. But, Curv is dynamically typed, the LLVM
  optimizations don't necessarily work very well.
* There is the potential to add support for optional strong typing so that you
  can write Curv code that matches the capabilities of LLVM, to be compiled into
  fast(er) code.
* Tail call optimization supported for Intel but not for Arm.
  This makes it a lot harder to generate code with guaranteed tail call
  optimization.

Consider (mostly) portable threaded code (of some sort).
* Faster compilation
* Slower evaluation
* The VM is potentially quite simple.

I'd like a design where first-class VM primitives can be implemented
as externally linked C++ code (as opposed to a VM where all primitives are
cases of a single switch statement):
* ordinary C functions, which are called by the VM
* macros which are invoked at compile time to generate VM code

Kinds of threaded interpreters:
* Direct threading.
  A 'thread' is an array of code addresses.
  An operation code block ends with 'jump *ip++'.
  Not compatible with operations as C functions.
* Indirect threading. Traditionally used by Forth.
  Has some benefits in convenience and compactness over direct threading.
  Code block ends with 'jump *(*ip++)'.
* Call threading.
  Operation has type `void (*op)()`.
  VM loop: `for (;;) (*ip++)();`.
  Operations are C functions.
  Probably a lot slower than direct threading (call vs jump).
  Real easy to code.

NaN coding.
Opcodes are NaN coded, either a literal value which is pushed,
*or* a code pointer. Can potentially have 3 bits of opcode.

VM operations:
* Constants are NaN coded, are their own opcodes.
* Non-constant list literal [a,b,c] is 'a b c 3 opMakeList'.
* Function call, function prototype known at compile time.
  Named arguments and missing arguments with defaults are processed at
  compile time. The correct # of arguments are pushed on the stack.
  The function is called. f(x,y) -> 'x y f'.
* Call to function whose prototype is discovered at run time.
  Push positional then named arguments onto stack. Push # of positional
  arguments and (sized) list of arg names, all determined at compile time.
  Push prototype, discovered at run time. Invoke opResolveArguments,
  which rearranges stack so that a full set of positional arguments
  is present. Invoke function.
* variable name
* return
* tail-call
* if-else
* let
* list comprehensions/generators
* function literal
