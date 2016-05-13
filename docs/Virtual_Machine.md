= Virtual Machine
Curv provides a syntax-independent backend for an OpenSCAD-like language.
It's a bit like LLVM. You write a parser which converts a source file
into a high level AST describing the semantics, not the syntax.
Curv provides a C++ API for constructing and evaluating the AST.
The evaluator contains a JIT compiler, which compiles the AST to vmcode.

== VMCode
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
* Slow compile times.
* It's exciting tech, may create interest in the project.
* A fast VM is a huge amount of work. So, leverage LLVM instead.
* Potentially supports native C function interface. Instead of passing
  arguments as array of curv::Value, pass them using native types.
* Need to learn a big API. Unknowns:
  * how to interface with existing classes like curv::Value and curv::ShPtr
    in generated code.
  * how to create debug interface
