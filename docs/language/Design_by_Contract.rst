Design by Contract
==================

"Design by Contract" is the idea that software components should have
a precisely defined "contract" for input and output behaviour.
The contract is verified at runtime, and code "fails hard" if the contract
is violated.

Curv makes it easy for functions to
specify and enforce a contract on their input and output.
And you can embed unit tests within a module, which test the functions
defined by the module to ensure they honour their contract.

The ultimate goal is to make Curv programs easy to understand and easy to debug
when something goes wrong.

Contracts for Functions
-----------------------
The contract for a function should be fully documented,
easy to describe and easy to understand.

If the contract is violated for a particular function call (bad arguments,
or it's impossible to construct a result which fulfills the contract), then
the the program is aborted with a meaningful error message and a stack trace,
making it easy to understand what went wrong and how to fix it.

Curv is a notation for executable mathematics.
Curv functions are mathematical functions.
The contracts of Curv functions are as much like their
mathematical counterparts as possible.

Curv is a pure functional language. All functions are pure, just like functions
in mathematics, which means that the result of calling a function depends only
on the argument value, and not on shared mutable state (which doesn't exist in Curv).
This makes function contracts simpler than in imperative languages.

In mathematics, functions obey algebraic laws.
For example, addition of two numbers is commutative (a+b==b+a),
associative (a+(b+c) == (a+b)+c), and so on.
Algebraic laws are an important tool for understanding and modifying programs,
and they are a way of expressing the contract of a function.
Curv functions obey the same algebraic laws as their mathematical counterparts,
to the greatest extent possible.

Example (Boolean Operations)
----------------------------
In mathematics, the boolean operations obey the laws of boolean algebra.

In many dynamically typed languages, all values are classified either as truthy (they count
as true in a boolean context), or falsey (they count as false in a boolean context).
In Javascript, for example, there are exactly 6 falsey values:
undefined, null, NaN, 0, "", and false. The rest are truthy.
In particular, [] counts as true, and new Boolean(false) also counts as true.
In Python, by contrast, [] counts as false.
It's hard to find two such languages that agree on the definitions of truthy and falsey,
so it's evident that something fishy is going on.

In these languages, the boolean AND operation (a && b) works like this:

1. If ``a`` is falsey, return ``a``.
2. If ``a`` is truthy, return ``b``.

In mathematics, the boolean AND operation is commutative: ``a && b``
is equal to ``b && a``. But AND is not commutative in these languages.
For example, 1 && 42 == 42, but 42 && 1 == 1.

In short, many dynamically typed languages have a complex and arbitrary contract
for boolean operations that breaks the laws of boolean algebra.

Curv has a simple contract for boolean operations, which obey the laws of boolean algebra.
The only boolean values are ``#true`` and ``#false``.
If a non-boolean value is passed to a boolean operation, the contract is violated,
and the program is aborted with an error message.

Enforcing Contracts in User-Defined Functions
---------------------------------------------
Curv provides a set of operations for enforcing contracts.
There are predicates for classifying values, and there are ways to report
an error if a contract violation is detected.

The following predicates return ``#true`` if the argument belongs to one
of the primitive types, otherwise ``#false``::

  is_bool
  is_num
  is_symbol
  is_string
  is_list
  is_record
  is_fun

``error`` *message_string*
  This is the primitive operation for reporting an error and aborting the
  program. A call to ``error`` can be used either in an expression context,
  or in a statement context.

  For example, let's consider a function ``incr`` which returns the successor
  of a number. The contract requires the argument to be a number::

    incr n =
      if (not(is_num n))
        error "incr: argument is not a number"
      else
        n + 1;

``assert`` *condition*
  An assert statement aborts the program if the condition is false,
  and does nothing if the condition is true.

  For example::

    incr n =
      do assert(is_num n)
      in n + 1;

``assert_error[error_message_string, expression]``
  Evaluate the expression argument.
  Assert that the expression evaluation terminates with an error,
  and that the resulting error message is equal to ``error_message_string``.
  Used for unit testing.

*value* :: *predicate*
  This expression is called a "predicate assertion".
  The left operand of ``::`` is an expression that evaluates to an arbitrary
  value. The right operand is a *predicate*, a function that returns true
  or false. If ``predicate(value)`` returns false, then the assertion fails,
  and the program is aborted.
  Otherwise, the predicate assertion returns ``value``.

  ``::`` is a low precedence, left associative binary operator,
  with the same precedence as ``>>``. This means ``:: predicate`` can be used
  as a component of a pipeline.

  In this example, we use a predicate assertion to enforce a post-condition
  on the return value of a function::

    incr n = n + 1 :: is_num;

*pat* :: *predicate*
  A predicate assertion can also be used as a pattern (eg, on the left side
  of a definition, or as a function formal parameter). This is a common
  use case. It is a terse way to enforce a contract on a function argument.
  
  If the predicate is false for the value being matched by the pattern,
  then the pattern match fails.
  Otherwise, if the predicate is true, then the value is matched against
  the subordinate pattern *pat*.

  For example::

    incr (n :: is_num) = n + 1;

``ensure`` *pred* *val*
  This is another syntax for predicate assertions.
  It's a curried function that is useful for tacit style programming.
  For example, ``map (ensure is_num)`` is a function asserting that its
  argument is a list of numbers.

  * If ``pred val`` is true, then succeed, and return ``val``.
  * If ``pred val`` is false, then fail.
  * If ``pred val`` doesn't return true or false, then panic.

  It can be used to enforce a post-condition on the return value of a function,
  with the predicate appearing at the beginning of the function::

    incr (n :: is_num) =
        ensure is_num <<
        n + 1;

Adding Unit Tests to Modules
----------------------------
A module is a set of definitions surrounded by braces::

  { incr x = x + 1; }

A *test definition* is a special kind of definition that
contains executable code (a statement), but it doesn't bind
any names. The syntax is ``test``\ *statement*. For example::

  test assert (incr 3 == 4);

The primary use case for test definitions is to represent unit
tests in modules. (But you can put test definitions in any
context where a definition is legal. And you can use test definitions
to embed debug code, such as print statements, in a definition list.)
For example::

  {
    incr x = x + 1;
    test (
      assert (incr 3 == 4);
      assert (incr (-1) == 0);
    );
  }

When the module literal is evaluated, the tests will be run.

The ``test`` keyword guarantees that the test is self contained: it cannot
modify state seen by other tests or seen by other parts of the program.
This means that you can delete a test without affecting the program's semantics.
Within a recursive definition context (like a module or a ``let`` phrase),
the relative ordering of test definitions doesn't matter.
Within a statement list (eg, inside a ``do``),
``local test <stmt>`` guarantees that ``<stmt>`` cannot
modify local variables from the surrounding scope.
