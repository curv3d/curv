# Range syntax

The OpenSCAD [a:b:c] syntax is ambiguous -- it means different things
in Matlab and in Python. The problem becomes worse if we extend this
to slice notation, eg vec[a:b:c], because then there is a serious conflict
with Python.

`a..b` is a reasonable syntax for the inclusive integer range from a to b,
because it looks like an ellipsis. It's the most popular syntax for this concept
in modern programming languages, but actually goes back to Algol68.
Similar syntax is being used in other popular languages, eg Rust and Swift.
It generalizes nicely to slice notation, where you optionally omit the beginning
or end of the range, eg
```
a[i..j]   a[i..]   a[..j]
a.[i..j]  a.[i..]  a.[..j]
a@(i..j)  a@(i..)  a@(..j)
```

How do you extend this syntax with an optional step value?
My solution is `a..b step c`. The `step` keyword is more readable
than any alternative I've tested using punctuation characters.
The `a..b` syntax is the closest I've found to a de-facto standard for
simple integer ranges, but once you add a step argument, there's no standard,
so any alternative using punctuation characters to designate the step is
unreadable.

It's difficult to extend this syntax with an optional step value.
The problem is finding a readable and intuitive syntax.
* Haskell uses `[0,10..100]`. I like this.
  * It doesn't work for slices if we also support multidimensional slices
    using `,` to separate axes.
  * It doesn't work if a list constructor contains a comma separated seqence
    of generator expressions, where a range is one such generator expression.
* One option is to treat `a..b` as an abbreviation of a more general syntax
  that supports strides, eg like `range(a, to: b, by: c)`.

Algol style: `for (i=[0 to 10 by 2]) ...`
This is fluent, readable.
Not as fluent for slices with optional endpoints:
```
a[i to j]   a[i to]   a[to j]
a@(i to j)  a@(i to)  a@(to j)
```

Rust:
* Range{start: a, end: b}
* a..b as shorthand
