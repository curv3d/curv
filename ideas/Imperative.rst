Imperative Style Programming in a Pure Functional Language
==========================================================
Curv is a simple, pure functional language designed for 3D modelling.
It is intended to be used by 3D printer enthusiasts, designers, and artists,
who have some exposure to computer programming. Users are not expected to
be software engineers or expert programmers.

There is a problem with creating a pure functional language that is targeted
at novice computer programmers. The programming languages that they learn
before encountering Curv will almost certainly be imperative languages like
Python, Javascript or Basic. If so, they will expect to write imperative
style code, using mutable variables and assignment statements. But
conventional pure functional languages do not have assignment statements,
and this is a major source of difficulty for novice programmers when they
first encounter these languages.

Curv solves this problem by providing an assignment statement, together with
standard imperative control structures: compound statements, ``if``, ``for``
and ``while`` statements. The assignment statement is restricted
to preserve the pure functional semantics of Curv, but the restrictions
are subtle enough that basic imperative idioms still work.

This feature also makes it easier to port imperative code to run in Curv.
You don't need to rewrite imperative algorithms in a functional style
(eg, converting while loops to tail recursion) before you can get the
code running.

Curv has list comprehensions. But instead of the idiosyncratic syntax used by
Haskell, Curv uses the same imperative control structures mentioned earlier
as the syntax for list comprehensions. It's another way of making the syntax
more friendly to imperative programmers.

Syntax of Curv
--------------
Here is the relevant subset of Curv syntax, which I'll describe first,
so that I can later explain the semantics. At the time of writing, Curv is
a work in progress, so this is the syntax as of March 2019.

Curv has expressions, definitions and statements.
An expression computes a value.
A definition binds a value to a name.
A statement has side effects.

Curv is an expression-oriented language. Programs are expressions.
Definitions and statements only occur as components of larger expressions.

A simple definition looks like this::

    x = a + b

This evaluates the expression ``a + b`` and binds the resulting value
to the name ``x``.

A ``let`` phrase defines local variables in the context of an expression
or statement::

    let <definition-sequence> in <expression>
    let <definition-sequence> in <statement>

For example::

    let a = 1; b = 2 in a + b

is an expression that yields the value ``3``.

Here are the relevant statement types:

``( <statement-sequence> )``
  A compound statement. Statements are separated or terminated using ``;``.
  Execute each statement in the sequence in left-to-right order.

``if ( <expression> ) <statement>``
  A one-armed if statement.

``if ( <expression> ) <statement> else <statement>``
  A two-armed if statement.
  
``while ( <expression> ) <statement>``
  A 'while' loop.

``for ( <name> in <list-expression> ) <statement>``
  A 'for' loop that iterates over a list value,
  binding each list element to ``<name>``
  as a local variable within ``<statement>``.

``<variable> := <expression>``
  Assignment statement. Assign a new value to a local variable, which
  was defined using ``let`` or ``for``.

A ``do`` expression executes some statements before evaluating an expression::

    do <statement-sequence> in <expression>

Here's an expression that uses imperative style programming
to sum the elements in a list L::

    let
        total = 0;
    in do
        for (elem in L)
            total := total + elem;
    in total

The syntax of Curv is optimized for functional programming, not imperative
programming. The biggest difference from modern imperative languages
is that local variable definitions are separated from the expression or
statement in which they are used by the ``let`` .. ``in`` construct.
This syntax works well for code written in the pure functional style, but
most popular imperative languages permit statements and variable declarations
to be freely intermixed within statement blocks. The other difference is
that ``name=expr`` is a variable definition in Curv, so I chose to use
``name:=expr`` as the syntax for the assignment statement to avoid ambiguity.
Most popular imperative languages use ``=`` as their assignment operator.
The Curv syntax is a callback to older imperative languages like Pascal,
Eiffel and Ada, which separate variable declarations from statements,
and which use ``:=`` for assignment. However, the users I wish to attract to
Curv are unlikely to be familiar with these older languages.
These syntactic differences could be viewed as a usability issue that needs
to be fixed (if the goal was to make Curv into a better imperative language),
or they can be viewed as a bridge or stepping stone to learning how to
program in a more functional style.

Semantics of Curv
-----------------
Mutable variables and the assignment statement are restricted so as to
preserve the pure functional semantics of Curv. Let's review those semantics.

* Curv has no shared mutable state.

* Curv is a "pure functional language", meaning that all functions in Curv
  are pure: they always map a given argument value onto the same result.
  (This follows from the absence of shared mutable state.)

* Expressions do not have side effects.

* Curv is "referentially transparent", which means that you can replace
  any expression with any other that evaluates to the same value.

* Curv supports "equational reasoning". You can use the laws of high
  school algebra to reason about the meaning of a Curv program. For example,
  in Curv, addition is commutative, which means that ``a+b`` and ``b+a``
  are equivalent: you can substitute one for the other without changing
  the behaviour of the program.

These features give Curv a simple and clear semantics
that make programs easy to understand for humans,
and also easy to compile into highly parallel GPU code.

Referential Transparency
------------------------
Within the functional programming community, "referential transparency"
means that any expression can be replaced by another expression that
evaluates to the same value, without changing the result of a program.
This ability to substitute equals for equals makes programs easier to
understand and modify.

To ensure referential transparency, Curv must abolish shared mutable state
and ensure that all functions are pure. Specifically,

 1. Functions may not modify shared mutable state.
    (More generally, function calls cannot have side effects.)
 2. Functions may not observe shared mutable state.
    (Therefore, the results returned by function calls depend only
    on the argument values.)

Normally, the presence of mutable variables and an assignment statement
would prevent a programming language from guaranteeing referential transparency.
To resolve this conflict, Curv places restrictions on these features.

To ensure property #1, the assignment statement is restricted to modifying
only local variables.

To ensure property #2, closures do not capture variable *references*,
they only capture variable *values*. A function can refer to a non-local
mutable variable X. However, when a function definition or lambda expression
is evaluated and a function value (called a closure) is created,
the closure captures the current value of X. If X is subsequently modified,
this has no effect on the closure, which remembers X's old value.

Assignment is Redefinition
--------------------------
Assignment is like defining a new variable with the same name. This new
variable hides the original variable for the remainder of its scope. If you
use this mental model for understanding the meaning of the Curv assignment
statement, then the restrictions that I am about to describe make more sense.

"Top level" assignment statement:

+---------------------------------+----------------------------------+
|this                             |is equivalent to                  |
+---------------------------------+----------------------------------+
|::                               |::                                |
|                                 |                                  |
|  let                            |  let                             |
|      x = 0;                     |      x = 0;                      |
|  in do (                        |  in let                          |
|      x := 1;                    |      x = 1;                      |
|      ... code that uses 'x' ...;|  in do (                         |
|  )                              |      ... code that uses 'x' ...; |
|                                 |  )                               |
+---------------------------------+----------------------------------+

Conditional assignment statement:

+---------------------------------+----------------------------------+
|this                             |is equivalent to                  |
+---------------------------------+----------------------------------+
|::                               |::                                |
|                                 |                                  |
|  let                            |  let                             |
|      a = f(x);                  |      a = f(x);                   |
|  in do (                        |  in let                          |
|      if (a < 0)                 |      a1 = if (a < 0) 0 else a;   |
|          a := 0;                |  in do (                         |
|      ... code that uses 'a' ...;|      ... code that uses 'a1' ...;|
|  )                              |  )                               |
+---------------------------------+----------------------------------+

Assignment within a loop:

+---------------------------------+-------------------------------------+
|this                             |is equivalent to                     |
+---------------------------------+-------------------------------------+
|::                               |::                                   |
|                                 |                                     |
|  let                            |  let                                |
|      total = 0;                 |      total = 0;                     |
|      i = 0;                     |      i = 0;                         |
|  in do                          |  in let                             |
|      while (i < count L) (      |      loop(total, i) =               |
|          total := total + L[i]; |        if (i < count L)             |
|          i := i + 1;            |            loop(total+L[i], i+1)    |
|      )                          |        else                         |
|  in total                       |            (total, i);              |
|                                 |      (total2, i2) = loop(total, i); |
|                                 |  in                                 |
|                                 |      total2                         |
+---------------------------------+-------------------------------------+

Restrictions on Assignment and Mutable Variables
------------------------------------------------

1. Functions do not capture variable references
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Imperative programming languages have mutable global variables which can be
accessed by functions. A function whose result depends on the
value of a global mutable variable is not *pure*: this function can return
different results for the same argument values.

This situation cannot occur in Curv. All functions are pure.
A function literal can reference non-local variables that are defined in a
scope surrounding the function. But functions do not capture non-local
*variable references*. Instead, when a function literal F is evaluated,
the current *value* of each non-local variable is captured. If one of those
non-local variables is later reassigned, it won't affect the behaviour of F.
This behaviour (functions capture variable values, not references) is
consistent with the "assignment is redefinition" picture of assignment
semantics.

2. Non-local variables cannot be assigned
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Imperative programming languages have mutable global variables which can be
modified by functions. When such a function is called, it has the side effect
of modifying shared mutable state.

Curv does not have shared mutable state, and functions do not have side effects.
Within a function, you can only assign local variables that are defined inside
the function body. Non-local variables are not assignable.

3. Variables have disjoint state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In Python, as in most imperative languages, it is possible for two variables
to refer to the same mutable object. For example, this Python program::

  a = [1,2]
  b = a
  a[0] = 42
  print(f"a={a}, b={b}")

will print ``a=[42, 2], b=[42, 2]``.
After the second line, ``b`` references the same mutable object as ``a``.
Consequently, the third line has the effect of modifying both ``a``
and ``b``.

And by the way, this behaviour can be confusing to beginning programmers.

Curv does not have shared mutable state, so this situation cannot occur.
The corresponding Curv program::

  let
      a = [1,2,3];
      b = a;
  in do
      a[0] := 42;
  in "a=$a, b=$b"

will return ``"a=[42,2], b=[1,2]"``.
Modifying the first element of ``a`` has no effect on ``b``.

[TODO: assignments like ``a[0] := 42`` are not implemented yet.]

4. Expressions have no side effects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Assignment statements have side effects,
and statements can be embedded in expressions, but these side effects are local,
and do not escape from the expression.

As a corollary, the order of evaluation of expressions
is not exposed by assignment statements.

An arithmetic expression like ``f()+g()`` is not guaranteed to be
commutative in imperative languages, because the subexpressions ``f()``
and ``g()`` might have side effects. Rewriting the expression as ``g()+f()``
might change the order of the side effects, which might change
the program's output.

The addition operator *is* commutative in Curv.
Curv is designed so that the order of evaluation of a function's arguments
cannot make a difference to the result of a program.

Here's a Curv program that attempts to use assignment statements in a way
that exposes the order of evaluation of the arguments to ``+``::

  let
    x = 1
  in
    (do x:=x+1 in x) + (do x:=x*2 in x)

In theory, this will return 4 if the arguments to ``+`` are evaluated
left-to-right, or it will return 3 if the arguments are evaluated
right-to-left.

In fact, this program is illegal, and you will get an error message.
If you consider the parse tree of this program, then between the variable
definition and the assignment statements, there is an operation (``+``)
which does not guarantee an order of evaluation, and that is not allowed.
You can not assign to a local variable V inside of an expression, if the
definition of V is outside of that expression.

List Comprehensions
-------------------
Curv reuses its imperative control structures as the syntax
for list comprehensions. This syntax is more friendly to imperative
programmers, as it is more familiar, and there is less overall syntax
to learn.

Let's consider a list comprehension in Haskell::

    [n | x <- [1..10], let n = x*x, n `mod` 2 == 0]

In Curv, this is::

    [for (x in 1..10) let n = x*x in if (mod(n, 2) == 0) n]

which yields ``[4,16,36,64,100]``.

The full statement syntax is available in list comprehensions, so you
can even use assignment statements and ``while`` loops.
F# uses a similar design.

Mutable Variables as Anaphora
-----------------------------
Inspired by this paper:
"Beyond AOP: Toward Naturalistic Programming"
https://www.cs.virginia.edu/~lorenz/papers/oopsla03a/oopsla03a.pdf

"Existing programming languages are based on the premise that each statement, expression or function is a little “black box” that relates to the rest of the program through an input-output interface. This premise is made very clear in functional programming languages that reduce everything, including other languages’ constructs, to functions. As a consequence, programmers are forced to stream their intentions into a series of sequential steps aligned with this very narrow pipeline view of the world."

"In this pipeline view of the world, there is no way of refining a statement or expression or function at a later point in the program text. Yet, this refinement happens pervasively in written discourse. The existing programming languages have a very shallow support for structural referencing and a complete lack of support for temporal referencing."

An important use case for mutable local variables is that you can first
define a variable by stating its value in the general or common case.
Then, in subsequent statements, you can refine the variable definition
(by modifying the variable's value) for special cases, by using conditional
assignment statements. This corresponds rather directly with how such things
are described using natural language. Subsequent references to the variable,
to refine the definition in special cases, are accomplished in natural language
using anaphora.

Of course you can rewrite the code using pure functional idioms, but the result
is less natural, and harder to understand, because it's farther away from how
we think and describe things using natural language.

Appendix: Referential Transparency
----------------------------------
The term "referential transparency" is used within the functional programming
community to describe the semantics of pure functional languages.
To understand what the term means, ignore Wikipedia and
read both of Uday Reddy's answers to `this stack exchange question`_.
It's commonly claimed that pure functional languages are referentially transparent,
and that imperative languages are not, but Uday Reddy demolishes this claim.

.. _`this stack exchange question`: https://stackoverflow.com/questions/210835/what-is-referential-transparency

I'm concerned that the term "referentially transparent" has become vague and incoherent,
so instead of using it to describe Curv, I've identified more specific and clearly
defined properties of the language to talk about (eg, that functions are pure, and that
expressions have no side effects).

Acknowledgements
----------------
Comments by Philipp Emanuel Weidmann (@p-e-w on github)
on a previous design for assignment helped to shape the current design.
