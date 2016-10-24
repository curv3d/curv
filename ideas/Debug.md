# Debugging

## Debugger UI
* Error handling: When a run time error is detected, we print the error
  message along with a stack trace, and abort the program.
  Optionally, you can enter the debugger and examine variables (post mortem
  analysis) before exiting.
* Cancel: a running script can be interrupted (eg with ^C).
* Suspend: You can interactively pause a computation (eg with ^Z),
  get a stack trace and examine variables, resume or cancel.
* Breakpoints: You can set breakpoints on functions,
  (which pause the computation when the function is called).
* Single step.

## Language Features
`echo`

## Runtime Debug Metadata
call frame contains debug metadata:
  * function value
  * pointer to previous call frame
  * optional RFunction/RThunk environment pointer?
  * maybe, access to argument locations, to store in bad argument exceptions.
    A call_phrase, used to print precise call location.

The debugger needs a way to inspect the lexical environment when the program
is paused in an arbitrary Meaning phrase. We need the ability to find a
symbol table for the lexical environment at a given meaning, and the ability to
look up run time binding values. It's too much to worry about right now.
* Due to nested `let` expressions, there are different local lexical
  environments for different Meanings within the same call frame.
  A brute force way to get symbols: When the debugger is first invoked,
  traverse the meaning tree and build a map from meaning pointers to symbol
  tables. Or, store an Environ pointer in each Meaning during analysis.

## Single Stepping
If I use a meaning tree interpreter, how does the debug interface work?
In particular, single stepping? Even if I use the tree representation,
I still need a virtual state machine with a PC and registers.
The tree node 'eval' function needs to advance the state.
Worry about this later, once I've worked through the requirements for the
analyzer, which works on the meaning tree.

## Implementation
You can register a debug callback functor with the interpreter (eval_script).
This gets called in the following situations:
* A fatal error is detected. The call stack is available during the call.
* An interrupt has been requested. The callback is called when the interpreter
  reaches a "safe point". The call stack is available. You can exit,
  single-step, resume.
* A breakpoint was reached or a single-step has completed.
### Stack Trace on Error
Store a list of call phrases in curv::Exception, to print stack trace.
Choices:

 1. Pass the top-of-stack around as `Frame*` arguments, and then into the
    curv::Exception constructor.
    * This is more functional and explicit, so maybe I have a better idea what
      the code is doing, and what the dependencies are?
    * Need to add `Frame*` arguments to:
      * Phrase::analyze (maybe put Frame* into curv::Environ)
      * `arg_to_*`
      * parser & scanner. Maybe put `Frame*` into curv::Scanner.

    Let's define the Context of an error to be the Location of the program
    text containing the error, plus the call stack (Locations of each call).
    Now, some functions like eval_file() might be called from the evaluator,
    where there is a context, or direct from the UI, which has no context.
    I need a representation for an optional Context, which can be passed
    as an argument to the Exception constructor, arg_to_string, eval_file, etc.
    I want to be able to write an expression which constructs either an
    empty Context, or a Context constructed from a Phrase and a Frame.
    So maybe `{}` and `{phrase,frame}`? Prefer to pass as `const Context&`.

 2. A thread-local variable points to top-of-stack. The curv::Exception
    constructor extracts the call phrase list from the stack by global
    variable reference, not by parameter. Less code.

This makes me think about the compile-time stack vs the run-time stack.
The compile-time stack only needs a new entry each time a new file is opened,
not like Environ. Currently this only happens in `file()` at run time, so
nothing special needed right now. If we implement `use module` then we'll need
to implement `file()` at compile time, but that's compile-time evaluation,
and then we'll push Frames at compile time. Which leads to a stack trace
with alternating zones of run-time and compile-time frames.
Later.

old
  - Frame contains a parent_frame and a call_phrase; latter is null for the
    base module frame. This forms a stack that can be dumped to print a stack
    trace.
    - Could store a list of call_phrases in the curv::Exception?
    - This could occur at the site where the original Phrase_Exception is
      thrown?
    - curv::Function::function_ now has a Frame* argument?
      And the top Frame could contain the argument list, the call_phrase
      (from which arg phrases are extracted) and the parent_frame?
    - In some cases, need to catch an exception (thrown from a library that
      can't be made Curv-specific), inject the stack trace, rethrow.
    - Maybe a thread-local variable points to the top-of-stack.
      - Exceptions are caught by the interpreter root function, inject stack
        trace, rethrow.
      - curv::Exception constructor could also query the thread-local variable.
      - Improves modularity? Not passing around frame pointers as much.
      - With the thread-local variable, we could avoid using the Frame for the
        call_phrase, and instead maintain a separate stack which is a list of
        Phrases, which specify the exception context. In some contexts, the
        top phrase is an arg phrase, followed by call phrases.
### Suspend on Error
On error, optionally enter the debugger, examine variables, stack trace
(post mortem analysis) before exiting. The debugger UI runs inside of the
debug() callback. The frame stack must exist when debug() is called.

We'd like a Frame to be associated with Function calls, so that they
show up in stack traces, and you can examine argument values in that Frame
using the debugger. Function arguments are passed in this Frame.
`Function::function_(Frame&)`.

How and when does debug() get called?

  1. Each Frame is owned by a local unique_ptr variable (current design).
     All the frames are destroyed when the curv::Exception propagate back up
     to the eval_script() call, so debug() must be called before the exception
     is thrown.
     * Could happen in the curv::Exception constructor.
->   * Or in an error() function that throws the exception.

     `curv::Function::function_` has a Frame* argument.
     `builtin_file` passes the Frame* to eval_file to eval_script
     to eval_module.

  2. A thread-local variable points to the top of the frame stack,
     using a unique_ptr. The Frame::parent_frame is also a unique_ptr.
     The stack therefore survives an exception being thrown.
     * eval_script catches the exception and calls debug?
       Remember that eval_script is called recursively via `file`.
       So only do this if the stack pointer is null when eval_script is entered.

Suppose a Function calls a library routine which throws an exception.
What happens?
 1. Function must catch exception then call error().
 2. No extra code required.
