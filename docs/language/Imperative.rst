Imperative Programming
======================
Curv supports imperative style programming by providing mutable local variables,
assignment statements, imperative control structures (sequencing, conditionals,
loops), a print statement (that prints to the debug console), and assertions.

Rationale
---------
Here are some reasons for using the imperative features:

* If you are learning Curv as a modelling language, and you don't wish to
  be a software developer, then imperative ``for`` loops and ``while`` loops
  are simpler and easier to understand than iteration coded in a pure
  functional style using tail recursion. Tail recursion is harder to
  understand; the use of auxiliary functions is inconvenient and adds clutter.
* Assignment statements are the simplest and most intuitive syntax for
  selectively updating elements of a nested data structure.
* Debugging using print statements.
* Embedding assertions and unit tests in your code.
* List and record comprehensions are expressed using the statement language.
* You are porting graphics algorithms from another language, and you don't
  want the hassle of translating from imperative to functional style.
  Preserving the imperative code structure makes it easier to track
  changes, and easier to reason about performance.
* For functions that execute on the GPU, such as signed distance functions,
  imperative loops are, at present, the only supported form of iteration.
  This is because we are compiling into GPU shader code, which does not
  support recursion. The good news is: with Curv, we can hide this
  imperative code inside a pure functional interface.

Curv provides a deliberately simplified subset of imperative programming.
The goal is to keep the language simple, and by that, I mean Curv has
simple and clear semantics. There is no "spooky action at a distance".
Everything in Curv (source code, data, program execution dynamics) has a
hierarchical structure and compositional semantics. You can understand a thing
using local reasoning, by understanding the meaning of each part in isolation.

Here are some consequences of this design principle:

* Imperative control structures conform to the principles of Structured
  Programming. They have a single entrance and exit. There are no gotos.
* Data is hierarchical. There are no pointers or object references, hence
  there are no cycles.
* Local variables are mutable, but values and data structures are immutable.
  When you update an element of a data structure using an assignment statement,
  it is only the contents of the local variable that changes.
* Functions are pure: the result of a function call depends only on the
  argument values. Function calls do not have side effects.
* Functions can only observe and modify their own mutable local variables.
  There are no mutable global variables. There is no shared mutable state.
* As a corollary, nested closures capture the *values* of local variables,
  they do not capture *references* to local variables.
* In an expression like ``a+b``, the order of evaluation of the operands
  doesn't matter. ``a+b`` is always the same as ``b+a``, no exceptions.
  From within the expression language, you cannot cause or observe side
  effects.

You should think of Curv as a simple language that combines the best parts
of pure functional and imperative programming, while leaving out the parts
that require non-local reasoning and make programs hard to understand.
