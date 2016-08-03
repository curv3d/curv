# Exceptions with Precise Location Reporting

1. Functions check for bad arguments, even primitive operations.
2. If a function call has a bad argument (domain error),
   then the location of the argument is reported in the exception.
3. The exception contains a stack trace.

How can precise location reporting be implemented efficiently?
I want fast compilation and fast evaluation in the normal case,
and only pay the cost of location reporting when an exception is raised.

## Argument Checking
For purely numeric operations, I can use Value::get_num_or_nan()
to construct the double arguments, then check for a NaN result:
```
    double r = x.get_num_or_nan() + y.get_num_or_nan();
    if (r == r) {
        // fast path
        return Value(r);
    } else {
        throw Exception("x+y: domain error");
    }
```
That should be faster than checking each argument first.

However, I don't report which argument was bad.

### For Pure Numeric Operations
Instead of detecting which argument was bad
(and reporting the location of the bad argument),
I could instead encode the argument values in the exception message.
```
throw aux::Exception(stringify(x,"+",y,": domain error"))
```
This assumes: aux::String is a ref-counted string,
aux::stringify returns an aux::Shared<aux::String>
(which has a noexcept copy constructor)
and, aux::Exception now takes aux::Shared<aux::String>
as constructor argument.

### Convenience in the Common Case
In most cases, I want a convenient API for converting unboxed values to
their required types, exiting on a type error.
Eg,
```
Shared<String> str = argv[0].get_str(); // throw exception on wrong type
int32_t i = argv[1].get_int(); // throw exception on wrong type
```

There is no location reporting in this API.

All functions parameters have names as part of their public API.
So, I could report the parameter name:
```
Shared<String> str = arg_to_str(argv[0], "str");
int32_t i = arg_to_int(argv[1], "i");
```

The exception could contain an informative message including the parameter
name, the expected type, the actual value. The stack trace that gets appended
to the exception value will contain the original function call, and with the
parameter name, that provides plenty of information.

### Efficient Location Reporting
Detect which argument was bad
and report the location of the bad argument.
Find a way to do this efficiently.

This mostly looks like a giant pile of LATER.

I have considered re-evaluating an arithmetic expression with
precise error checking if it returns a NaN. Could use the tree evaluator.
This requires all inputs to the arithmetic expression to be preserved until
the final result is available, which may prevent register reuse.

Rerun entire script from scratch if an exception is thrown, but this time,
use the tree evaluator with precise location reporting.
Or, pick restart points so that they don't introduce much overhead.

I have considered using signalling NaNs, then mapping the machine state
to an exception. This is ambitious: messing around at the machine level.
Google "precise exceptions", since knowing the PC at the time of the signal
may itself be difficult.

Some (but not all) of numeric argument checking is type checking.
Type annotations and type inference can hoist type checks out of a loop,
which makes them cheaper.

## Argument Location
The unboxed function call API is `Value apply(Value*)`.
There's no location information about the arguments.
This is possible: `Value apply(Value*,Location*)`,
but it adds cost at compile and run time.

In terms of what's easy, a primitive non-numeric function will throw
an exception with the name of the bad argument.
Ditto for user defined functions with parameter assertions,
such as `(x:is_num)->...`.
This can be combined with information from the stack trace
to reproduce the location.

## Stack Trace
If an exception is raised, then the exception contains a stack trace.
Maybe a linked list of Locations?
If a stack trace entry contains the boxed function value, then
we can map parameter names to locations (see above).

What's in the stack trace?
All operator expressions and function calls?
It seems likely that 'primitive operations' will be treated specially,
for efficiency reasons, with less precise location reporting,
while each 'general case function call' will appear in the stack trace.

The stack trace is stored in the curv::Exception so that the Bad_Argument
exception can access it to reproduce the Location.
A stack trace entry is a linked list node containing an Apply_Phrase
and a Function value. Both are needed to reproduce the location of a bad
argument. The Apply_Phrase alone reproduces the location of the function call.

How do I construct the stack trace?
* Most likely: A chain of try-except handlers at each call point.
  Code bloat; overhead is low (eg 5%) until exception is thrown.
  Supposedly: one extra branch usually not taken + lost optimizations.
  What's the cost for LLVM? Itanium ABI "zero cost" exception handling.
  Windows is more expensive. This is complicated in LLVM, especially if
  Windows support is needed.
* a VM register points to a stack of cons cells `(Location*,next*)`,
  pushed/popped at each call point.
  * thread local variable, or part of Eval_Context
* Use libunwind, glibc/osx backtrace(), but this just gives native function
  addresses; Location info is harder to access and very platform dependent.
  But: no run time overhead. Encode location key into function name?

The stack trace doesn't contain tail calls.

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
