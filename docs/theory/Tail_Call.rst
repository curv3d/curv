Tail Call Optimization
======================
Within the body of a function ``f``,
a tail call is a function call (eg, ``g(x)``)
that is performed as the final action of ``f``
before returning the value ``g(x)``.

Tails calls are important in functional programming
because they can be implemented more efficiently than
regular function calls. A tail call can be implemented without
adding another stack frame to the function call stack.
You can think of a tail call as a form of "goto" that jumps to
another function, while passing arguments.
This means you can use tail calls to efficiently implement iteration.

Here is an example in Curv::

  let
    fac_loop[n,a] = if (n <= 1) a else fac_loop[n-1,n*a];
    factorial n = fac_loop[n,1];
  in
    factorial 10

There are two tail calls in this program.
The first is the recursive tail call ``fac_loop[n-1,n*a]``
which implements the tail-recursive loop that computes a factorial.
The second is the body of the ``factorial`` function itself: ``fac_loop[n,1]``.
This means that the function call ``factorial 10`` only requires a single stack
frame on the function call stack.

In the functional programming community,
efficient implementation of tail calls is considered to be
a fundamental requirement for functional programming languages.
Curv meets this requirement.

Definition of a Tail Call
-------------------------
A tail call is a function call that appears in a tail context.
A tail context is defined as follows:

* The body of a function is a tail context.
* If ``if A then B else C`` appears in a tail context,
  then ``B`` and ``C`` are also tail contexts.
* If ``let A in B`` or or ``B where A`` or ``do A in B``
  appear in a tail context, then ``B`` is also a tail context.

Shortened Stack Traces
----------------------
Although TCO is a requirement for functional languages, it is controversial
in the imperative programming community. Lua does it. But Guido doesn't want it
in Python (he considers it "unpythonic"). The Javascript standards committee
tried to add it to Javascript ES6 in 2015, but there was a developer rebellion:
Google implemented it in Chrome, then reverted the change.

What could be the problem?
TCO eliminates frames from the call stack, which means that it shortens stack
traces that are used for debugging. Adding TCO to an existing imperative
language is a non-starter for this reason: after the change, stack traces
have less information, which the existing dev community sees as a loss. Few
people support the change, because nobody is writing the kind of functional
code that benefits from this change. They can't, because without TCO, tail
recursive loops exhaust the stack and crash your program.

On the flip side, if you actually do write code in a functional style, using
tail recursive loops, then you don't want each iteration of a loop to appear
in your stack traces. You get huge stack traces that are full of noise.

Curv has TCO, which means stack traces are shorter than they would be
without TCO. If this turns out to be a real problem for debugging,
we will investigate ways to mitigate it.

Note that it's really easy to suppress TCO on a case-by-case basis.
Just define::

  id x = x

and wrap tail calls like ``f(x)`` that you want to appear in a stack trace
using ``id(f(x))``.

Limitation: the Shape Compiler
------------------------------
The Shape Compiler does not support tail call optimization.
This is because none of the current and future code generation targets
support this (the list includes GLSL, C++, WebGPU, WebAssembly).

The Shape Compiler also doesn't support recursive function calls,
because GLSL and WebGPU don't support this. Therefore, you can't use
tail recursive loops in any function that will be compiled by the Shape
Compiler. You must instead use imperative `for` and `while` loops.

Can we do better than this in the future? Yes, but it's complicated.

The first step is to support self-recursive tail calls, by converting
them into ``while`` loops. Elm does this when compiling to Javascript.
(Elm doesn't support mutually recursive tail call optimization, however.)

On the GPU, ``while`` loops are often slower than ``for`` loops, especially
loops of the form ``for (i = k1; i < k2; i += k3)``,
where ``k1``, ``k2`` and ``k3`` are constants. This latter form of ``for`` loop
is recognized as a special case by GPU shader compilers, which inline the
loop if the number of iterations is small enough.

So the second step is to optimize the ``while`` loops that we generate into
``for`` loops when that is feasible. Without this optimization, developers will
still need to avoid self-tail-recursion in favour of imperative style loops.

A possible third step is support general tail recursion for mutually recursive
functions. This is pretty complex, and I don't know another language
that attempts this. You would need to identify groups of functions that
recursively call each other (a mutual recursion group),and inline expand all
of the functions of each group into a single uber-function. Each uber-function
has an outer ``while`` loop, and uses variables and a state machine to keep
track of which function is currently executing within the uber-function.

This use of ``while`` loops and variables to simulate a state machine is
slower than hand-written imperative code, especially on a GPU. So you want
to optimize this code to get rid of the variables. This is possible, but
quite complicated. This is equivalent to converting a control flow graph
(CFG) into high level structured control statements (if, while, for) using
an algorithm like Relooper or Stackifier. Emscripten does this, but "The
Relooper is the most complex module in Emscripten". Cheerp uses an improved
version of Stackifier, which generates efficient code, faster than Relooper.

References
----------
* This is the original paper explaining the usefulness of the tail call optimization.
  Guy Steele, 1977.
  "Debunking the “Expensive Procedure Call” Myth; or, Procedure Call Implementations Considered Harmful; or, Lambda: The Ultimate GOTO."
  `<https://litrev.files.wordpress.com/2009/07/aim-443.pdf>`_
* Scheme was the first programming language that required implementations to be *properly tail recursive*.
  See `<https://people.csail.mit.edu/jaffer/r5rs/Proper-tail-recursion.html>`_.
* Tail recursion elimination is "not Pythonic", so it is not supported by Python.
  `<http://neopythonic.blogspot.com/2009/04/tail-recursion-elimination.html>`_
* The Emscripten "Relooper" algorithm. See section 3.2 of:
  Alon Zakai, 2011.
  "Emscripten: An LLVM-to-JavaScript Compiler".
  `<https://github.com/kripken/emscripten/raw/master/docs/paper.pdf>`_.
* The LLVM "Stackifier" algorithm.
  `<https://reviews.llvm.org/D12735>`_
* Cheerp: improved Stackifier consistently produces faster code than Relooper.
  `https://medium.com/leaningtech/solving-the-structured-control-flow-problem-once-and-for-all-5123117b1ee2`_
