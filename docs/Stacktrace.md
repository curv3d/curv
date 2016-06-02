If an exception is raised, then the exception contains a stack trace.
So, like, a linked list of Locations (to start with).
Whatever expression was evaluating, then function calls.

How do I construct the stack trace?
* a VM register points to a stack of cons cells `(Location*,next*)`,
  pushed/popped at each call point.
  * thread local variable, or part of Eval_Context
* A chain of try-except handlers at each call point.
  Code bloat; overhead is low (eg 5%) until exception is thrown.
  Supposedly: one extra branch usually not taken + lost optimizations.
  What's the cost for LLVM? Itanium ABI "zero cost" exception handling.
  Windows is more expensive. This is complicated in LLVM, especially if
  Windows support is needed.
* Use libunwind, glibc/osx backtrace(), but this just gives native function
  addresses; Location info is harder to access and very platform dependent.
  But: no run time overhead. DEFER

Regardless of technique, can't wait until stack is unwound to get trace.
* Which means code at each call point to construct call stack.
* Which means code at point of exception throw to extract backtrace.
* DEFER: Or highly platform dependent code to hook exception mechanism.

So there's a VM register pointing to a stack of cons cells
containing `(Location*,next)` pairs. The register could be a thread-local
variable, or it's part of Eval_Context, which needs to be passed when an
exception is thrown. Or use libunwind. Or a chain of try-except handlers
at each stack frame.
* Do native functions throw non-curv exceptions that need to be decorated?
  That seems hard, there's no way to copy a std::exception. Although I can
  grab the what() string. (In LLVM generated code, I have more control!)
* If I use a thread-local variable for the location stack, then I can use
  a single exception handler at root of evaluator (no, C stack is already gone)
* throw an exception on error. put a try-except at each point where a stack
  frame needs to be added to the exception. Overhead is low (eg 5%) until
  an exception is thrown.
    try {
        eval();
    } catch (std::exception& e) {
        decorate_and_rethrow(e, location);
    }
* glibc/osx backtrace(), libunwind, ...
* Use std::set_terminate() to set a terminate handler that is called when
  a program is about to abort due to unhandled exception, obtain back trace.
  No good, this requires the process to be terminated (not just the thread).
* If I were programming in Rust, then what? No 'exceptions', just abort the
  thread (with some control during unwind) or return error values.
  There is rt::backtrace to get current stack trace. There are no magic
  features here I can't get from C.

```
ERROR: something bad happened
#1: foo/beeswax.scad:42:7
  bad(x) + 1
  ^-----
#2: foo/beeswax.scad:17:7
  be_bad(who);
  ^----------
```
