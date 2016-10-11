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
  * maybe, access to argument locations, to store in bad argument exceptions

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
