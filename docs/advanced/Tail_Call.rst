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
    fac_loop(n,a) = if (n <= 1) a else fac_loop(n-1,n*a);
    factorial n = fac_loop(n,1);
  in
    factorial 10

There are two tail calls in this program.
The first is the recursive tail call ``fac_loop(n-1,n*a)``
which implements the tail-recursive loop that computes a factorial.
The second is the body of the ``factorial`` function itself: ``fac_loop(n,1)``.
This means that the function call ``factorial 10`` only requires a single stack
frame on the function call stack.

In the functional programming community,
efficient implementation of tail calls is considered to be a basic requirement
for functional programming languages.
Curv meets this requirement.

Shortened Stack Traces
----------------------
Although TRO is a requirement for functional languages, it is controversial
in the imperative programming community. Lua does it. But Guido doesn't want it in Python
(he considers it "unpythonic"). The Javascript standards committee tried to add it to Javascript ES6
in 2015, but there was a developer rebellion: Google implemented it in Chrome, then reverted the change.

What could be the problem?
TRO eliminates frames from the call stack, which means that it shortens stack traces
that are used for debugging. Adding TRO to an existing imperative language is a non-starter
for this reason: after the change, stack traces have less information, which the existing dev
community sees as a loss, and nobody is writing
the kind of functional code that benefits from this change, because without TRO, tail recursive
loops don't work: your program exhausts the stack and crashes.

On the flip side, if you actually do write code in a functional style, using tail recursion
for loops, then you don't want each iteration of a loop to appear in your stack traces.
You get huge stack traces that are full of noise.

Curv has TRO, which means stack traces are shorter than they would be without TRO.
If this turns out to be a real problem for debugging,
we will investigate ways to mitigate it.

Limitations: the Shape Compiler
-------------------------------
The Shape Compiler does not support tail call optimization.
This is because none of the current and future code generation targets
support this (the list includes GLSL, C++, SPIR-V, WebAssembly).

It is theoretically possible for the Shape Compiler to support tail call
optimization for recursive calls (the case that matters most), but without
target support, it is very complex and difficult to do so in the general case.
You could identify groups of functions that recursively call each other
(a mutual recursion group), inline expand all of the functions of each group into
a single function, use variables and a state machine to keep track of which function
is currently executing. The resulting code would be quite slow on a GPU, so you want
to optimize the code and get rid of the variables. GLSL and SPIR-V don't have goto
statements, so you need to convert a control flow graph (CFG) into high level structured
control statements (if, while, for) using an algorithm like Relooper or Stackifier.
Emscripten does something like this, but "The Relooper is the most complex module in Emscripten".

Until somebody implements this complex algorithm in Curv, you will need to implement
your loops imperatively using ``for`` and ``while``, in Curv code that is
compiled by the Shape Compiler.

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
