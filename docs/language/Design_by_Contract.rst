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

pure functions

algebraic laws

Operations
----------

::

  is_null
  is_bool
  is_num
  is_string
  is_list
  is_record
  is_fun

  ensure pred expr = do assert(pred expr) in expr;
  assert
  error

