Functions
---------
A function ``f`` is a value that maps an argument ``x`` onto a result ``f x``.

Functions are pure. The result of a function call depends only on its argument
value. (In other words, a function cannot consult shared mutable state to
compute its result. This is a feature of imperative programming languages,
and Curv is a pure functional language.)

Functions with One Argument
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Function call is a binary operator with two operands, the function and
its argument, which are juxtaposed or separated by a space. You can write
``f(x)``, but the parentheses are not a required part of function call syntax.

``identifier -> expression``
  This is a function literal.
  For example, ``x->x+1`` is a function that maps its argument ``x``
  onto the value of ``x+1``.
  If ``incr=x->x+1`` then ``incr 2`` is ``3``.

Functions with Multiple Arguments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
There are 3 ways to simulate a function call with more than one argument.
Let's consider a function called ``plus`` which takes 2 arguments
and returns their sum. Here are 3 variants:

* ``plusl [2,2]`` -- The argument is a list.
  This simulates a conventional function call with positional arguments.
* ``plusr {x:2, y:2}`` -- The argument is a record.
  This gives us function calls with labelled arguments.
* ``plusc 2 2`` -- This is called a Curried function.
  How it works: ``plusc 2`` returns a function that maps its argument ``y``
  onto ``2+y``.

Let's define the 3 variants of ``plus``::

  plusl = [x,y] -> x + y;
  plusr = {x,y} -> x + y;
  plusc = x -> y -> x + y;

The definitions of ``plusl`` and ``plusr`` use the patterns ``[x,y]``
and ``{x,y}`` as their parameter specification.
The pattern ``[x,y]`` requires the argument to be a two element list:
otherwise, the pattern match fails.
It binds the list elements to the parameters ``x`` and ``y``.
Similarly, the pattern ``{x,y}`` requires the argument to be a record with
exactly two fields named ``x`` and ``y``. It binds those field values to
the parameters ``x`` and ``y``.
See also: `Patterns`_.

There is a shortcut syntax for function definitions::

  plusl [x,y] = x + y;
  plusr {x,y} = x + y;
  plusc x y = x + y;

Definitions are described in more detail here: `Blocks`_.

Pipelines
~~~~~~~~~
Curv programs often contain deeply nested function calls,
which happens when you chain a sequence of geometric transformations.
Pipelines are an alternative syntax for nested function calls, which are
easier to read and write.

The following shape expression::

  colour red (rotate (45*deg) (cube 10))

can be rewritten as a pipeline::

  cube 10
   >> rotate (45*deg)
   >> colour red

You read a pipeline from left to right, like a Unix shell pipeline.
Take a cube of size 10, then rotate it 45 degrees, then colour it red.
The data flows from left to right, through a series of transformations.

Briefly, ``x >> f`` means ``f x``.
There is also a reverse pipeline operator, ``f<<x``, which is less frequently used.

``into``
~~~~~~~~
``into`` is a preposition for writing pipelines.

``a >> into f [b, c]`` means ``f [a, b, c]``.

More generally, ``a >> f list`` means ``f [a, ...list]``.

``into`` is used for interjecting a call to ``union``, ``intersection``
or ``difference`` into a shape pipeline.
For example:

+--------------------------------+----------------+
| ::                             | |dodeca-icosa| |
|                                |                |
|   dodecahedron                 |                |
|    >> colour red               |                |
|    >> into union [icosahedron] |                |
+--------------------------------+----------------+

.. |dodeca-icosa| image:: ../images/dodeca-icosa.png


Infixes
~~~~~~~
``a `f` b`` is an alternate syntax for ``f[a,b]``.

Infixes are a way to extend Curv with more numeric operations
that are written in infix form, just like the built in numeric operators
such as ``+`` and ``-``.
Some functions which look good when used in infix form:

+--------------------------------------+-------------------------------------+
| ``a `mod` m``                        | ``mod[a,m]``                        |
+--------------------------------------+-------------------------------------+
| ``a `max` b``                        | ``max[a,b]``                        |
+--------------------------------------+-------------------------------------+
| ``l1 `concat` l2``                   | ``concat[l1,l2]``                   |
+--------------------------------------+-------------------------------------+
| ``v1 `dot` v2``                      | ``dot[v1,v2]``                      |
+--------------------------------------+-------------------------------------+

Match
~~~~~
``match`` implements a multi-branch conditional using pattern matching.

``match function_list``
  ``match[f1,f2,...]`` constructs a new function ``f`` whose argument can match
  any of the parameter patterns in ``[f1,f2,...]``. To evaluate ``f x``, check
  if ``x`` matches the parameter pattern of ``f1``. If so, return ``f1 x``.
  If not, continue with the remaining functions in the list. If no function
  accepts the argument ``x``, then the pattern match for ``f x`` fails.

This is like an overloaded function definition in other programming languages.

For example, let's define an "overloaded" version of ``plus`` that accepts any
of the 3 argument patterns from the previous example::

  plus = match [
    [x,y] -> x + y,
    {x,y} -> x + y,
    x -> y -> x + y,
  ];

Then, ``plus[2,2]``, ``plus{x:2,y:2}`` and ``plus 2 2`` all yield ``4``.

In the definition of ``plus``, ``x->y->x+y`` must be the final list element,
because the parameter pattern ``x`` matches any value,
and any following list elements would be ignored.

Functions with Record Fields
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A record with a ``call`` field is treated like a function.
Suppose we define::

    hello = {
        call: x -> "Hello, $x!",
        description: "The hello world function"
    };

Then we the function call ``hello "Fred"``
will return the value ``"Hello, Fred!"``.
This is equivalent to writing ``hello.call "Fred"``.

One way of thinking of this feature is that a record with a ``call``
field is a function with additional user-defined attributes.

Other Operations
~~~~~~~~~~~~~~~~
``compose function_list``
  Function composition.
  ``x>>compose[f,g,h]``
  is equivalent to
  ``x>>f>>g>>h``.

``is_func value``
  Returns true either if the argument is a primitive function like ``x->x+1``,
  or if it is a record with a ``call`` field for which ``is_func`` is true.
  Otherwise returns false.

  All of the primitive operations on functions, including function call
  syntax, ``match``, and ``compose``, use ``is_func`` to determine if an
  argument is a function.

``is_primitive_func value``
  Returns true if the argument is a primitive function.
  Returns false if the argument is a record with a ``call`` field.
  This predicate should only be used in those rare cases where you need
  to classify the set of all values into disjoint categories.

.. _`Patterns`: Patterns.rst
.. _`Blocks`: Blocks.rst

Domains and Errors
~~~~~~~~~~~~~~~~~~
The domain of a function is the set of argument values that it accepts,
and maps onto valid results, without reporting an error. The domain is
part of the function's contract: it's something you document for users
of the function.

There are two ways that a function call can terminate and report an error:
it can fail, or it can panic.

 * Failure means that the argument is not a member of the function's domain:
   the caller has violated the function's contract by passing a bad argument.
   A failure is a recoverable error: it can be converted into
   a pattern matching failure (allowing other patterns to be matched instead).

 * A panic is a nonrecoverable error. Due to a problem detected at runtime,
   the function is unable to honour its contract. A panic may indicate
   data structure corruption, or a logic error in the program. It could
   also indicate an error in the Curv virtual machine, such as resource
   exhaustion. A smart compiler could detect some panics at compile time,
   and report them as compile time errors.

When an ordinary function call such as ``f x`` fails, the program aborts
with an error message. The top of the stack trace is the failing function
call ``f x``, because that's where the problem is: the caller passed a bad
argument to the function.

When an ordinary function call panics, the program aborts with an error message.
But the stack trace is different: it indicates the subexpression within the
function's body where the panic occurred. In order to decode the stack trace,
you need to understand the function's implementation.

In practice, whether a bad argument (not in the function's domain) is
reported as a failure or a panic depends on how you implement the function.
It is desirable to report bad arguments as failures, rather than as panics,
because the stack trace is easier to understand, and you can use failures
in pattern matching.

The general syntax for a function literal is::

    pattern -> expression

The argument is matched against the pattern, and if the pattern match fails,
then the function call fails. So pattern matching is the primary mechanism
for reporting failure in a function call.

Using ``match`` and ``compose``, you can construct a function with a complex
domain by combining functions with simpler domains.
However, this means using hardcore functional programming idioms.
Designing the best coding style for reporting domain
errors as failures remains an active area of research.

Here are the detailed semantics of ``match`` and ``compose``.

The ``match`` function is the fundamental primitive for conditional evaluation.
It maps a list of functions ``[f1,f2,...]`` onto another function ``f``.
To evaluate ``f x``, we first try ``f1 x``. If that succeeds, return the result.
If that panics, then panic. Otherwise, if that fails, then try ``f2 x``
next. If all of the functions in the list have been tried, and none of them
succeed, then ``f x`` fails. The domain of ``f`` is the union of the domains
of the argument functions.

The ``compose`` function is the fundamental primitive for sequential evaluation.
It maps a list of functions ``[f1,f2,...]`` onto another function ``f``.
``compose[f1,f2] x`` is similar to ``x >> f1 >> f2``, except that if either
``f1`` or ``f2`` fail, then the composed function also fails. The domain
of ``f`` is the intersection of the domains of the argument functions.

The ``id`` function always succeeds. ``id x`` always returns the value ``x``.
It never fails, because its domain is the set of all values.

The ``error`` function always fails: its domain is the empty set.

Match and compose form an algebraic structure over function values.
It's actually a "non-commutative semiring", with ``match`` playing the
role of addition, ``compose`` playing the role of multiplication,
``error`` playing the role of ``0``,
and ``id`` playing the role of ``1``.

 * Match is an associative function with ``error`` as the identity.

   * ``match[error,f]`` is ``match[f,error]`` is ``f``.
   * ``match[]`` is equivalent to ``error``.

 * Compose is an associative function with ``id`` as the identity.

   * ``compose[id,f]`` is ``compose[f,id]`` is ``f``.
   * ``compose[]`` is equivalent to ``id``.

 * Multiplication by zero yields zero. That is,

   * ``compose[f,error]`` is ``compose[error,f]`` is ``error``.

 * Product distributes over sum. That is,

   * ``compose[a,match[b,c]]`` is ``compose[a,b] `match` compose[a,c]``
   * ``compose[match[a,b],c]`` is ``compose[a,c] `match` compose[b,c]``
