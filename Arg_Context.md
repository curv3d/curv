# The Argument Context

Function call now has a single argument,
which may be destructured by pattern matching.
A parenthesized argument list is no longer part of function call syntax.

So now, what is the exception context for 'argument n' of a function call?

If the function has only one argument, you use
    At_Arg(f)
    At_GL_Arg(f)

If the function has two arguments, then it really has one argument which
is a 2-list. You specify which argument as a list index:
    At_Arg(0,f)     At_Arg(1,f)
    At_GL_Arg(0,f)  At_GL_Arg(1,f)

In the future, we'll support a sequence of indexes. Not for release 0.0.

How is a "bad argument" error printed?
We can no longer underline argument i, because of generators, etc.
So, we indicate which argument failed in the message.

[from Arg(f)]
true: not a list
--underline the argument part of the function call--

[from Arg(0,f)]
At argument[0], "foo": file not found
--underline the argument part of the function call--


This is now trickier because the Call_Phrase argument isn't guaranteed to be a
list literal, so we might not be able to underline the bad argument any more.

 1. Give up. Just print the call stack using At_Frame(f).
    Specify which argument failed in the message. 
    Although that's hard if you use Value::to<type>(context).

 2. Okay, maybe the context needs to specify which argument.
    So a context specifies a location stack and an optional argument index.
    We print the location stack, and include the argument index in the message
    as needed. Eg,
        argument[1]: value is not a number
        ...call stack...
    This sounds feasible for release 0.0.

 3. Get fancy. Put a list of argument indices into the context.
    Not for release 0.0.
 
 4. Get fancy. Highlight just argument 'i', if possible, rather than just
    showing the complete function call.
    Difficult, because even if the argument is a list literal, we don't know
    which elements are generators.
    Not for release 0.0.

## The Fancy Option
You create an argument context acx. Then you can push an index, pop an index,
set the top index. Hopefully this is not too expensive. You are doing work on
each iteration of a loop.

Alternatively, try to recover the index path from your runtime state at the
point where an exception occurs. A bit tricky if recursion is used.

This isn't happening for release 0.0.
