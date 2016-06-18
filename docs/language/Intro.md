# SCode
Scode is a dynamically typed, pure functional programming language.
It is small, simple and elegant, with 7 basic data types.
It has a small implementation, designed for being embedded in another project.

SCode is a somewhat simplified and idealized variant of OpenSCAD2,
and I'm building it in part as a technology demo for the OpenSCAD
community, to demonstrate future possibilities for that project.

SCode will be the scripting language for Red Mercury, which is another
technology demo.

## Implementation
I should use C++ as the implementation language. That's what OpenSCAD is written in,
and is the best choice if I want the OpenSCAD project to use my code,
or likely if I want others to use my code.
That gives me compatibility with libraries like LLVM, and is a good option
if I want to steal code from other similar language implementations,
eg like Lua, or one of the Javascripts.

I'm probably going to need garbage collection, though. I'm not convinced
that reference counting will be adequate.
* I could generate code (byte codes? threaded code?) for a VM that 
  uses a garbage collector.
  * Is there another VM that I can just steal? I don't have a good answer yet.
* But I also want to just do something quick and dirty for the first prototype.
  I might throw scode away; no sense in investing too much time in a VM.
  This suggests a simple syntax tree interpreter plus a GC that I don't have
  to write myself. Eg, use the Boehm GC.
* I could also use a GC'ed language that provides good C/C++ compatibility
  so that I can export a C and maybe export a C++ API.
  * The D language looks okay. It's #21 on the 2015 language popularity list.
  * Nim is too immature.

Red Mercury and OpenSCAD have a GUI. If your script goes into an infinite loop,
there needs to be a way to terminate the evaluation thread without leaking
resources or corrupting memory.
* Running the evaluation thread in a separate process kind of sucks,
  because now the evaluation result has to be transmitted across a process boundary.
* In C++, it is impossible to safely terminate a thread without the thread's
  cooperation. This leaves me with periodically checking to see if thread
  termination is requested, at safe points during evaluation. Ugly and slow.
* Can pthread_cancel be safely used in C++?
  In general, no: https://skaark.wordpress.com/2010/08/26/pthread_cancel-considered-harmful/
* There is no thread cancellation in C++11, Rust, D.
* I'll have the same problem in Red Mercury: rendering might take forever,
  and needs to be safely interruptible. Seems I need cooperative thread cancellation.

If a script runs too long, you ought to be able to suspend it, and debug it.
Eg, get a stack trace, examine values of variables.
This is a future requirement, if the project gets past the prototype phase.
* Compile to a VM, and put debug support in the VM.
  Whatever mechanism the VM uses to support suspension of evaluation
  will likely also support termination (see above).
* Build an abstraction on top of ptrace?? Sounds hideous, but I'd be
  able to get stack traces for native functions.

Red Mercury will want to compile SCode to optimized machine code using LLVM
for in-cpu rendering.
* how to terminate?
  * This code is likely constrained so that it can't allocate memory
    (or do non-tail recursion). I could rely on these restrictions to
* how to debug?

## Is Refcounting Sufficient?

Impact of a tracing GC:
* In Google V8 (Javascript) API, there's a heavy interface for "handles"
  that reference values in the Javascript GC'ed heap.
  This complicates the code that references the CSG tree.
  It complicates native code called from within the SCode interpreter.
* Boehm collector fixes this, at a performance cost.
* Apple JavaScriptCore uses a generational Bartlett collector
  that also addresses the "handle" problem for native code:
  https://webkit.org/blog/3362/introducing-the-webkit-ftl-jit/
* Maybe use refcounted values in SCode, if I can make it work.
  It is not obvious how.
* If I use a garbage-collected language (D?) then I'd like it to use
  a GC that doesn't require handles for referencing values in C
  functions called from the language.

Can SCode values be refcounted? Functions and objects are trickiest.
What's the impact of recursive definitions?

Maybe a function value is an array of values of non-local bindings, plus the code.
In the simplest naive implementation, a recursive function definition will
lead to a function value containing cyclic references.

The Y combinator is a way to define a recursive function without cyclic refs.

Recursive definitions only occur in an object literal. (Not in let(...).)
- Each function definition in an object is implemented by passing
  a 'this' argument to the compiled function, by which other fields in the object
  are referenced. This supports include and override semantics.
- A function value contains:
  * a reference to the value of the smallest enclosing object literal.
    These are linked in a chain by parent pointers.
  * a vector of non-local variable values captured by the function,
    inside the smallest enclosing object literal.
  * a pointer to the function code.
- Does this eliminate cycles in the representation of recursive functions?

What about laziness in object literal bindings?
That involves thunks, which can involve cyclic references?
Thunks are encoded like functions. They *only* appear in definiens
in an object def: problem solved. No lazy lists or lazy function calls.

I think this design works, but it could fall apart if somebody tries to hack
in some advanced new feature without understanding. Garbage collection might
be more robust in the face of hacking? Well, somebody could screw up the code
generator and leave off a barrier or something.

Refcounting is potentially more useful to the OpenSCAD devs, since it's what
they currently use and understand.
