= Design By Contract

Eiffel introduced the term "design by contract", and uses the
'require', 'ensure' and 'invariant' keywords to express preconditions,
postconditions and class invariants.

preconditions: `require(condition)expr`

postconditions: `ensure(predicate)expr`

module invariants: `assert(condition);` as a statement within a script.
Assertions are evaluated in source code order during the evaluation of
a module expression, and when a module is customized,
before element expressions are evaluated.

function parameter assertions: `(name:predicate)->body`.
Similar to `name->require(predicate name)body` except that
the error message highlights the function call argument
that failed the assertion.
Parameter assertions are also available in let and module bindings.

New syntax for parameter assertions:
    (Predicate parametername)->body
eg,
    (Num x)->x+1
is like
    x->(assert(Num x); x+1)
Benefits:
    * it's terser
    * it frees up the valuable `:` character for something else
    * it follows my abbreviation design principle in that the `Num x` form
      is preserved in the abbreviation.
But
    * might suck if predicate is long and complicated.
      (Use reverse funcall operator in that case?)
