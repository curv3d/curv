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

References
----------
* This is the original paper explaining the usefulness of the tail call optimization.
  Guy Steele, 1977.
  "Debunking the “Expensive Procedure Call” Myth; or, Procedure Call Implementations Considered Harmful; or, Lambda: The Ultimate GOTO."
  PDF: `<https://litrev.files.wordpress.com/2009/07/aim-443.pdf>`_
* Scheme was the first programming language that required implementations to be *properly tail recursive*.
  See `<https://people.csail.mit.edu/jaffer/r5rs/Proper-tail-recursion.html>`_.
* The Emscripten "Relooper" algorithm. See section 3.2 of:
  Alon Zakai, 2011.
  "Emscripten: An LLVM-to-JavaScript Compiler".
  PDF: `<https://github.com/kripken/emscripten/raw/master/docs/paper.pdf>`_.
* The LLVM "Stackifier" algorithm.
  `<https://reviews.llvm.org/D12735>`_
