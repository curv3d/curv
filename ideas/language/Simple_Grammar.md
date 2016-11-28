Can the grammar be simplified?
* Simple yaccable grammar with no conflicts or precedence declarations.
* More of the syntax moved into metafunctions: `let` and `for`

## Everything is a Chain
Suppose that `let` and `for` are metafunctions.
Their tail arguments are chains, which consist of primaries, dots and calls,
no symbolic operators.

`if` is changed to be consistent; the `then` and `else` phrases are chains.
If these phrase contain symbolic operators, they'll need to be parenthesized.
`if` binds more tightly than infix operators, so
    if (x>y) x else y + 1
groups as
    (if (x>y) x else y) + 1
which is consistent with the precedence of regular chains.
    foo(x) bar(y) + 2
But this precedence for `if` contradicts C syntax, ML/Haskell syntax, etc,
so it will be troublesome.

I could eliminate the dangling else ambiguity by forbidding the `then` phrase
to be a naked `if` phrase.

if (x>y) x else y + 1

## Greedy Chain Syntax
Explicit syntax for greedy chains.
* probably an operator symbol like f << x or f : x
* maybe this symbol also works for greedy if-else?

## Primary Chain Syntax
The "consistent" syntax for if/let/for/function calls is that the tail
argument is a primary expression.
chain
  : primary
  | if <primary> <primary> else <if-or-primary>
  | let <primary> <primary>
  | for <primary> <primary>
  | chain <primary>

So
* 'if(a)b else if(c) d' is legal, but no dangling else ambiguity.
* `if` could be implemented as a metafunction. Weird but possible.
* `let` and `for` can be defined as metafunctions.
