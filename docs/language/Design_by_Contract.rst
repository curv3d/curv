Design by Contract
==================

"Design by Contract" is the idea that software components should have
a precisely defined "contract" for input and output behaviour.
The contract is verified at runtime, and code "fails hard" if the contract
is violated.

The goal is to make Curv programs easy to understand and easy to debug
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
The only boolean values are ``true`` and ``false``.
If a non-boolean value is passed to a boolean operation, the contract is violated,
and the program is aborted with an error message.

Enforcing Contracts in User-Defined Functions
---------------------------------------------
Curv provides a set of operations for enforcing contracts.
There are predicates for classifying values, and there are ways to report
an error if a contract violation is detected.

The following predicates return ``true`` if the argument belongs to one
of the primitive types, otherwise ``false``::

  is_null
  is_bool
  is_num
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
      if (!is_num n)
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

*predicate* *pat*
  This is a compound pattern which can be used as the formal parameter
  of a function. It is a terse way to enforce a contract on a function argument.
  
  The *predicate* is a function that returns true or false.
  If the predicate is false for the value being matched by the pattern,
  then the pattern match fails.
  Otherwise, if the predicate is true, then the value is matched against
  the subordinate pattern *pat*.

  For example::

    incr (is_num n) = n + 1;

``ensure`` *pred* *val*
  This is an expression asserting that ``pred val`` is true,
  and then returning ``val`` if the assertion succeeds.
  It can be used to enforce a post-condition on the return value of a function::

    incr (is_num n) =
        ensure is_num << n + 1;
