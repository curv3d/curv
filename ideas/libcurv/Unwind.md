# Unwinding the stack
The stack must be unwound when handling an error, or when printing a stack
trace in the debugger. How is this implemented?

Requirements:
* Error handling: When a run time error is detected, we unwind the stack,
  releasing reference values and accumulating a stack trace.
  Then we print the error with its stack trace.
* Cancel: a running script can be interrupted (eg with ^C).
  This requires unwinding the stack and cleaning up.
* Suspend: You can interactively pause a computation (eg with ^Z),
  get a stack trace and examine variables, resume or cancel.
* Breakpoints: You can set breakpoints on functions,
  (which pause the computation when the function is called).
* Standards-conforming foreign function interface:
  * C++ functions called from Curv code can throw exceptions to report errors.
  * Foreign functions can be cancelled or suspended without using non-standard
    APIs.

## Current Design Proposal
There are tradeoffs between compile time, execution time, and difficulty
of implementation.
* A "byte code" interpreter offers fast compiles and slow execution. For typical
  CSG oriented scripts, this might be the fastest way to build a CSG tree.
  Also, this gives fast feedback if there is an error in the script.
* LLVM MCJIT is slow for compilation. It offers the possibility of very fast
  execution, but achieving this is quite difficult, especially with my special
  requirements for stack unwinding, debug, tail call elimination.
* For my first prototype, I should use a simple byte code interpreter.
  I can easily achieve all of my required functionality without compromise,
  and without getting mired in LLVM hell. Once I have an LLVM implementation,
  I might still need a byte code interpreter anyway.

I'll use direct threading: it's generally considered to be faster than the
alternatives: call threading, switching on integer opcode values.
This requires the "labels as values" extension,
so I'll need gcc or clang to build Windows executables.
* Erlang interpreter uses direct threading. Ruby interpreter uses direct
  threading by default (can be configured to use other implementations).
* April-2014: Clang generates slow code for direct threading:
  <http://llvm.org/devmtg/2014-04/PDFs/Talks/drejhammar.pdf>
* Travis-CI, ubuntu 14.04 doesn't support c++14 by default. There is clang.
  Or: apt-get install g++-5. On Mac, Clang is only option.

I'll use two stacks. The data stack is an array of curv::Value, which allows us
to find and release all Values when an exception is thrown. The call stack
is an array of call records, containing a return address and location metadata.
A stack trace can easily be generated from the call stack at any time.
Maybe there is also a data stack frame pointer, plus # args in the metadata,
so that the arguments can be printed in a stack trace.

Error handling: An error is reported by throwing an exception. The exception
is caught at the top level. The interpreter stack will have a well defined
state: I can extract a stack trace, and release resources.

Cancel: We periodically check a global `request_interrupt` variable and throw
an error if it is true. `pthread_cancel()` works this way. For compatibility
with C, a foreign C function could use `pthread_testcancel()` to test for
a pending interrupt.

Suspend: The console UI and the evaluator are separate threads. Can't use Unix
signals to stop a specific thread; ditto for ptrace. So we implement
co-operative suspend, similar to cancel. At each control point, the interpreter
checks a thread-local variable to see if it is time to cancel or suspend.

Breakpoints: Could be implemented as conditional suspend. Or by overwriting
an instruction in a function header with the trap instruction.

## Implementation Considerations

### Cancel
We can't just immediately cancel a thread at an arbitrary machine instruction.
The problem is that the C and C++ library functions called from Curv don't
support this. Gcc and the C/C++ ABI used on intel Linux can support this,
if used correctly, but existing libraries that we must link with do not.
* In C, for example, cancelling a thread in the middle of malloc() will likely
  corrupt the heap.
* Even in C++, raising an exception at an unexpected location can cause bad
  behaviour. The code might not be exception safe (eg, a library written by
  Google). Or, if the exception is raised inside a destructor while unwinding
  the stack to handle an exception, then std::terminate is called.

So we need a co-operative thread cancel API. We periodically check a global
`thread_is_cancelled` variable and throw an exception if it is true. This is
performed at "cancel points", where it is safe for an exception to be thrown.

Blocking I/O operations are an issue (importing a resource from a URL):
these operations ought to be interruptible. And that can be implemented
using Unix signals (eg, SIGINT).

It would be nice if there were a standard or de-facto standard API for
co-operative thread cancellation. Then, we'd have a chance of convincing
third party libraries (like CGAL) to call this API from long-running functions.
(Or, we ask the third party library to add their own propriety thread-cancel
hooks, to avoid dependency on an external API.)
* `pthread_cancel()` and `pthread_testcancel()` looks like it should be a
  candidate. Unfortunately, system calls like `close()`, that might be called
  from a C++ destructor, are cancellation points, and may spontaneously
  "throw an exception" (unwind the stack and clean up). And that could lead
  to `std::terminate` being called. At least, that was a danger in 2010:
  <https://skaark.wordpress.com/2010/08/26/pthread_cancel-considered-harmful/>
* It's possible to add code to a destructor to protect against this, but the
  problem is that existing C/C++ libraries can't be assumed to support this.
  So maybe you need to wrap calls to C/C++ library routines that don't support
  `pthread_cancel` with code to disable/reenable cancellation. Which is
  terrible.
* Boost has interruptible threads. And it doesn't hook signal handling to turn
  blocking system calls into cancellation points, like pthreads does, so it is
  safer to use.

Another possibility is: run the computation in a thread, and on interrupt,
detach the thread and allow the UI to be responsive while the abandoned thread
runs to completion. Problems:
* The computation thread might be in an infinite loop. We still need a way
  to interrupt interpreter loops, even if interrupting third party library
  calls is problematic.
* `aux::Shared` is not thread safe.
  * Maybe we keep track of whether the thread is currently "in the interpreter"
    or "in a long running third party library function" (like CGAL union).
    But how does that help? After it returns, we still need to clean up
    interpreter resources (eg the Value stack), which needs to happen
    synchronously.
  * Maybe there's some kind of async programming structure that supports
    multiple simultaneous computation threads, but only one can be in the
    "critical section" for manipulating reference counts at a time.
    This could allow multiple CGAL operations to run in parallel.
  * I have a design for shared objects that are born thread-unsafe, but
    which can optionally graduate to being thread-safe. Maybe that helps?
    * Not all Values would be shared between threads. The concern is
      the builtin values and the atom table. So make those thread safe?
      * I could abandon the Session when a computation is interrupted, and
        make a new one. Which takes care of the atom table.

Interrupt is a special case of error handling: we periodically check
a global `request_interrupt` variable and throw an error if it is true.
* `pthread_cancel()` works this way. For compatibility with C, a foreign C
  function could use `pthread_testcancel()` to test for a pending interrupt.
  But: pthread_cancel is not safe in C++. The problem is that cancel points
  can invisibly occur in destructors due to design of the Posix C library,
  which leads to destructors throwing exceptions on thread cancellation,
  which leads to std::terminate being called. Eg, `close()` is a cancel point.
  <https://skaark.wordpress.com/2010/08/26/pthread_cancel-considered-harmful/>
  (This sounds like a platform bug. Did it ever get fixed?)
* Boost has an API for interruptible threads. That's the closest to a de-facto
  standard I can find. Doesn't solve the problem of interrupting blocking
  system calls (eg, those that do I/O), which pthread_cancel addresses.
  I/O is an issue for importing a resource using a URL.
* Cancellation takes effect at cancel points: just before a foreign function
  call, just before a loop iteration or recursive tail call, just before
  a function call.
* In principle, we could have an instruction level unwind map that allowed
  us to unwind and clean up from any execution state, which would eliminate
  the runtime overhead of checking a global variable. GCC has
  `-fasynchronous-unwind-tables` which seemingly provides the necessary
  information.
  * In practice, if we are currently executing a foreign (C)
    function, eg from an externally linked library, we probably don't have
    the needed unwind tables. To start, we'd have to compile all external
    objects linked to the Curv library specially.
  * A 2009 forum post indicates that this flag only supports unwinding from
    instructions that generate traps, not from arbitrary instructions.
    <https://gcc.gnu.org/ml/gcc-help/2009-10/msg00228.html>
  * But gcc documentation: Generate unwind table in dwarf2 format, if supported
    by target machine. The table is exact at each instruction boundary, so it
    can be used for stack unwinding from asynchronous events (such as debugger
    or garbage collector)
* From above URL:
  The fastest trick is to do what Sun's Java does:
  have a special page in memory. `volatile int *my_page = mmap one page;`
  In your loops, do this: `int foo = *my_page;`.
  When you need to interrupt a loop, turn off read access to that page.
  You'll get a segfault from which you can throw an exception.  This is
  safe when compiling with `-fnon-call-exceptions`, which allows trapping
  instructions to throw C++ exceptions.

### Suspend
Suspending a computation?
* Maybe, we only support pausing at "suspend points", which are likely the
  same as cancel points. From such points, metadata is available to show a
  stack trace or query variable values.
  * How does UI access execution state? I guess there are two threads.
* Maybe we have instruction level pausing, implemented like in gdb.

With a Meaning tree interpreter, this is easy: all of the needed metadata
is accessible from Meaning nodes.

If we compile to byte codes or machine code, then:
* We generate unwind tables mapping instruction positions to metadata which
  describe how, from that point, to unwind or traverse the call stack.
  Also need to query variables.
* Or, we generate code that queries for external events and procedurally
  embeds metadata.
  * Error handling: a function fails by returning a boxed error value,
    which is tested for and propagated up the call stack.
  * Cancel: test for cancellation at cancel points, then return an error value.
  * Suspend: test for suspension at suspend points. Then, set up a shared
    data structure that exposes execution state (stack trace and variables),
    block eval thread while UI thread reads this data structure.
  * Stack trace: maintain a call stack linked list data structure, including
    metadata pointers, so call stack and variables can be queried.
  * Breakpoints: there's a breakpoint boolean variable for each function
    that can be tested when it is called, in function prolog.

## Cost of Implementation
* cost of writing the code
* cost of compiling a script
* cost of execution

well?
* A stack based byte code interpreter would be easy to write, compile fast,
  execute slowly.
* LLVM using MCJIT and unwind tables would be slow to write and compile,
  would execute the fastest.

Fast evaluation isn't the most important thing for evaluating the script.
The byte code interpreter will still be faster than OpenSCAD.
The debug features will be golden, and shipping sooner is also important.

The distance field will need to be compiled to GPU or machine code; that's
a limited subset of the language.

## Error handling in the Curv VM

It seems that catching C++ exceptions within JIT generated machine code
is problematic. So I need an alternative approach.

1. JITted code cannot call functions that throw exceptions.
   My JIT generated code will be ignorant of C++ exceptions, and will not
   clean up (by decrementing reference counts) if an actual C++ exception is
   thrown. C++ functions called by curv JITed code must be declared `noexcept`,
   which requires C++11.

2. A curv function fails by returning an error value.
   Error values are automatically propagated up the call stack.
   (At each stack frame, the function call location is added to the error value,
   so that it accumulates a stack trace.)

So `curv::Error` is a subclass of `curv::Ref_Value`.

I need fast code to check a return value and return it back up the call stack
if it is an error. Return values are 64 bit values, normally, and the check
will quickly test that bit string, without a cache hit.

In the case of curv::Value, we'll use a special tag value that can be
quickly tested. I'm thinking of 0x7FF7'0000'0000'0000 as the 16 bit prefix
for a 48 bit pointer in a reference value.

I'll use likely/unlikely macros for the error test.

The cost of my scheme is a comparison test and an untaken
branch after calling each function that might return an error.
I hope this is as fast or faster than the cost of the Itanium C++ ABI for
exception handling, in the case no exception is thrown. If not, I can copy
ideas from that ABI. My situation is simpler and more constrained, so I have
potentially room to be faster.

(This has made me think of compiling curv with exception handling and RTTI
disabled, for better performance. LLVM also does this. I could also make
heavy use of `noexcept` for performance.)

## Unwind Tables
There's a bunch of stuff I don't understand about "unwind tables"
that are generated to support C++ exception handling and also to
allow debuggers to generate a stack trace.

So how does this work? Is it better than what I've designed?

What is a Dwarf Unwind Table? When gcc generates code that handles exceptions, it produces tables that describe how to unwind the stack. These tables are found in the .eh_frame section. The format of the .eh_frame section is very similar to the format of a DWARF .debug_frame section.

The .eh_frame section is a sequence of records. Each record is either a CIE (Common Information Entry) or an FDE (Frame Description Entry). In general there is one CIE per object file, and each CIE is associated with a list of FDEs. Each FDE is typically associated with a single function. The CIE and the FDE together describe how to unwind to the caller if the current instruction pointer is in the range covered by the FDE.

This shit is used by glibc's backtrace() function.

Each CIE may contain a pointer to a personality function,
and each FDE may contain a pointer to the LDSA, the Language Specific
Data Area. Each language has its own personality function.
The LDSA is only used by the personality function, so it in principle could
be different for each language. (In GCC, every language uses the same format.)

I want to associate location data with each function call.
Maybe I can define a Curv-specific LDSA?
* Sounds ambitious. It will definitely require use of the MCJIT api,
  which will slow down compiles. It will prevent JITed code from catching
  C++ exceptions, but that was my starting point for this exploration.
  No need to worry about Windows compatibility.
* Actually, the Itanium unwind API is compatible with multiple languages
  with different design decisions. So a C++ exception can unwind a Curv
  stack, invoke Curv cleanup code, create a Curv stack trace.
