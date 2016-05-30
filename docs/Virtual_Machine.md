# Virtual Machine
Curv provides a syntax-independent backend for an OpenSCAD-like language.
It's a bit like LLVM. You write a parser which converts a source file
into a high level AST describing the semantics, not the syntax.
Curv provides a C++ API for constructing and evaluating the AST.
The evaluator contains a JIT compiler, which compiles the AST to vmcode.

## VMCode
VMCode is the virtual machine code.
The VM also has registers and an execution model.

Implementation choices:
* Integer opcodes (stack or register), threaded code, or LLVM.
* Should I model x+y as an 'operation' and sin(x) as a 'function call',
  or are all operations with function semantics treated as functions?
  Integer opcodes implements common operations as opcodes, making them fast,
  while slower operations are encoded as function calls.

some rough ideas and requirements:
* debug interface, stack trace, break, single step, examine variables.
* can safely cancel execution of a script.
* simple C++ interface for coding native operations. Define functions,
  create and query managed values.
* it would be nice if vmcode doesn't priviledge a particular set of operations
  and data types over others, so that Curl can be reused in more situations,
  so that the Curl API is more universal, and Curl internals don't need hacking
  for straightforward language extensions like new types and operations.
  The Lua and Javascript VMs bake in design decisions about types and
  operations that make them not a great fit for Curv; can I do better?
* proper tail recursion optimization

I should consider LLVM as my backend.
* Potentially very fast runtimes (with enough work).
* Conforming to LLVM could have payoff later for GPU code generation, which has
  a similar model. I could target GLSL, or SPIR-V, or a mooted LLVM SPIR-V
  target.
* Slow compile times.
* It's exciting tech, may create interest in the project.
* A fast VM is a huge amount of work. So, leverage LLVM instead.
* Bad: tail call optimization supported on Intel but not Arm.
  The IR is basically C, not ASM. Can't branch to a function? Can't define
  a function with multiple entry points? Options:
  * Special case for self-tail-call: use a branch or loop.
  * Special case for mutually recursive functions bound in the same object,
    which can be detected by compiler.
    * Compile these into a single function that has the union of the parameter
      lists, plus a selector parameter specifying which function is called.
      Internally, tail call is a branch.
      Externally, each function has an entry point which calls the combined
      function.
    * Or, inline the code for a tail call to a mutually recursive function.
  * GLSL doesn't support recursive functions. Converting tail recursion into
    loops could have payoff for a GPU implementation. Maybe tail recursion is
    the only recursion supported for distance functions.
  * General case: use a trampoline. Function call op works like this:
    a function returns either a value or a thunk continuation. If thunk is
    returned, call it and loop. Expensive, continuation is heap allocated.
* Potentially supports native C function interface. Instead of passing
  arguments as array of curv::Value, pass them using native types.
* Need to learn a big API. Unknowns:
  * how to interface with existing classes like curv::Value and curv::ShPtr
    in generated code.
  * how to create debug interface

