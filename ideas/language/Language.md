# The Curv Core Language

This describes the core of the Curv language, absent geometric shapes and
geometric operations.

## Types
The Curv core language is a dynamically typed, pure functional language
with 8 types of values.
There are 6 data types, which correspond to the JSON data types:
null, boolean, number, string, list, record.
There are 2 code types, which contain compiled code: function and module.
Everything is a first class value.

There are no type names or user defined data types.
The closest we get to a type name are predicate functions which test
a value and return true or false, depending on whether they belong
to a particular "type".

## Error Handling
What happens when a function is unable to compute a result
(eg, because it is passed bad arguments)?
In Curv, there are two cases:
* Abort, by printing an error message and terminating the program.
* Return `null`, which is a unique value indicating failure or the
  absence of a result.

Curv functions are very strict about detecting bad arguments and
either aborting, or returning `null`. The default behaviour, absent a
good reason to return `null`, is to abort.

By contrast, OpenSCAD is extremely forgiving about bad arguments, and
will usually either ignore them, or return null, but it won't abort.
This creates problems:
* If you make a mistake, OpenSCAD usually won't tell you, and you have to
  go searching through the code. This is particularly frustrating for
  beginners.
* It is technically impossible to add new semantics to an existing function,
  because every change is a breaking change. People can and do write code
  that passes bad arguments to built-in functions or modules,
  and relies on whatever random behaviour results.

By being hyper strict about bad arguments, Curv helps people find bugs
in code. If we discover that a particular interface is too strict, we can
relax the interface without breaking anything, but going in the other direction
creates upgrade problems by breaking existing code.

## The null value
The special value `null` indicates the absence of a result, and is returned
when a function needs to indicate that it couldn't compute a result.

The `null` value is identified with `undef` in OpenSCAD.

When OpenSCAD code is executed by Curv, operations that normally return NaN
will actually return `null`. However, native Curv numeric operations
abort with a domain error if they can't compute a number.
For example, OpenSCAD `sqrt(-1)` returns `null` while Curv `sqrt(-1)` aborts.

Unlike NaN, `null` doesn't mess with semantics of the equality operator.
So `null==null` is `true`. This is so you can easily test if a function
has returned `null`.

## Equality
`x == y` tests if the two values `x` and `y` are equal, returning a boolean,
while `x != y` tests for inequality.

Equality is defined for all values.

Equality is an equivalence relation, which means:
* `x==x` (reflexive)
* if `x==y` then `y==x` (symmetric)
* if `x==y` and `y==z` then `x==z` (transitive)

It's tricky to make equality work for function values.
* It's no good to simply compare pointer values of the underlying function
  objects. That would lead to a behaviour change if the common subexpression
  optimization were applied to a lambda expression: a compiler optimization
  could break existing code. Worse, if we had a profile based JIT compiler,
  then the semantics could change during a program run.
* Suppose `f` is either a function value, or `null`.
  It should be possible to test this using `f==null`,
  which means it's no good for equality to abort if one of the arguments
  is a function.
* The simplest design that works is for all functions to compare equal
  to each other, and to compare unequal to non-functions.
  I don't think that anything more sophisticated than this is needed.
  Function equality is not particularly useful.

Here are some aspirational guidelines for how equality should work:
* If two values have the same printed representation when echoed,
  then they are equal.
* If two values are operationally equivalent, then they are equal.

It's infeasible to compare two functions for operational equivalence:
technically this is not computable, and even an approximation is not
worth a complex implementation. The first guideline suggests
that all function values be printed as "<function>" when converted to
a string.

We could extend this to print "<function NAME>" in cases where
the function is the body of a function definition, or just "<function>"
for anonymous function literals. This is a bit more useful (for debugging),
It's still fairly safe: it can't be messed up by compiler optimizations.
It's a bit nasty in that two operationally equivalent functions can print
differently; it could expose internal implementation details of a library,
so that a library change that doesn't otherwise affect operational behaviour
could still affect this printing behaviour, and thereby *break existing code*.
This exposes a tension between the need for clean abstract semantics,
and the need for debugging features that expose the implementation.
Maybe debugging can be kept outside of the language, and restricted to the
debugger interface. If debug annotations need to be added to source code, maybe
they can be clearly marked, and prevented from affecting the model geometry.

## Debugging
There is an interactive debugger interface. Initially a console UI like gdb.
You can pause a computation, get a stack trace, resume, set breakpoints.

You can add "debug annotations" to the code that print trace output,
or break into the debugger, conditionally or unconditionally.
These annotations:
* are highly visible in the code, distinct from the model description.
* have no effect on the model geometry.
* have access to extra language features that expose implementation details.
* are perhaps compiled out for performance reasons if the debugger interface
  isn't active.

The 'echo' command is an example of a debug annotation.
Possible example of a debug annotation syntax:
```
    debug {
	sequence of definitions and debug statements
    } expression
```
I've occasionally wanted to set a breakpoint at source location B
conditional on specific data values being detected earlier at location A.
Maybe there's a command to set a breakpoint.

## Monoids
A monoid is a binary operation which is associative, and which has
an identity value. There are many examples of monoids in OpenSCAD,
including addition, multiplication, min, max, concat, union, intersection.

In Curv, every monoid is implemented as a function of a single argument,
which is a list of values. The monoid operation is applied repeatedly
to reduce the list to a single value. If the list is empty, then
the identity element is returned. In addition, there may optionally be an
infix operator that implements the monoid.

For example, `max` is a monoid (the identity value is `-infinity`),
so instead of writing `max(a,b)` as in OpenSCAD, you write `max[a,b]`,
and you write `max(list)` to find the maximum value in a list.

For another example, `concat` is a monoid (the identity value is `[]`),
so instead of writing `concat(a,b)` as in OpenSCAD, you write
`concat[a,b]`, and you write `concat(list)` to flatten a list.

For a third example, addition is a monoid (the identity value is 0),
so in addition to the infix `+` operator, we also provide the function `sum`,
which sums a list.

The fact that these functions all operate on a single list argument
becomes more important in the context of arrays, described later.

Curv does not support varargs functions, not even built-in ones.
In all cases where a variable number of arguments is needed, a single
list argument is used instead.
* This is more flexible, because the list argument can be written either as
  a list literal, or as an expression which computes a list at run time.
  There's no loss of convenience, because you can write
  `max[a,b]` instead of `max([a,b])` as an abbreviated form of function call.
* It's more consistent. All functions have a fixed number of arguments,
  with a different name for each argument. Labeled arguments work consistently
  for all functions.
* It's more efficient. Since functions are first class values, we have
  to pick a uniform calling convention. A varargs calling convention,
  where all arguments are pushed on the stack, and the number of arguments
  is also passed, is inefficient. Instead, functions are compiled into machine
  code using a C style calling convention with a fixed number of arguments,
  some of which may be placed in registers. Tail recursion optimization is not
  possible for varargs functions, but is possible for our calling convention.

## Boolean values
There are two boolean values, `true` and `false`.
Functions that expect a boolean value will abort if a non-boolean is passed
instead: that's considered a type error.

Many dynamic languages permit non-booleans to be used in a boolean context,
but no two languages agree on the meaning. In Python, 0 counts as false,
while in Ruby, 0 counts as true.

Curv's strict design addresses some common problems in OpenSCAD.
For a beginner, it's easy to accidently type `cube(10,20,30)`
or `square(5,10)`, instead of `cube([10,20,30])` or `square([5,10])`.
OpenSCAD's behaviour of ignoring errors doesn't help beginners find these
bugs. It turns out that `square(5,10)` is equivalent
to `square(size=5,center=10)`. The `center` argument is supposed to be
boolean, and in this context, `10` counts as false. In Curv,
`square(5,10)` will abort with an error, since the second argument
(the `center` argument) is non-boolean. If Curv were to behave as
some have advocated, and it were to
consistently treat 0 as false and non-zero numbers as true, then
`square(5,10)` would not report an error, instead it would center the square.

*bool1* `&&` *bool2*
> `true` if both arguments are true, otherwise `false`
> It's traditional to short-circuit the evaluation of *bool2* if *bool1*
> is false.

`all(list)`
> The function version of the `&&` monoid: return true if all list elements
> are true. Return true if the list is empty.
>
> Eg, `all[for (x=L) x>0]` is true if all list elements are > 0.
> Note: `all` stops processing its list argument when the first false element
> is found (short circuit evaluation). In the example, due to laziness,
> the `for` loop will exit when the first non-positive x is found.

*bool1* `||` *bool2*
> `true` if either argument is true, otherwise `false`

`some(list)`
> The function version of the `||` monoid: return true at least one list
> element is true. Return false if the list is empty.
> Note: `some` stops processing its list argument when the first true element
> is found.

`!` *bool*
> logical negation of argument

`if (`*bool*`) `*val1*` else `*val2*
> conditional expression

## Numbers
Curv numbers are 64 bit IEEE floating point numbers, just like OpenSCAD.

Numeric literals have the same syntax as C/C++.
I'd like to support `'` as an optional digit group separator (like in C++14).
For example, `1'000'000`.

Numeric operations don't return NaN, they abort instead.

`infinity`
> `1 / 0 == infinity`

Curv has the same set of numeric operations as OpenSCAD,
with a few differences.

`mod(x,m)`
> modulus.
> Unlike `%` in OpenSCAD, `mod` correctly computes the modulus for both positive
> and negative arguments.
> ```
> mod(x, m) == x - m*floor(x/m)
> ```
> Reference: http://mathworld.wolfram.com/Mod.html

*x*` ^ `*y*
> raise *x* to the power of *y*

Trigonometry: `sin`, `cos` etc take arguments in radians, rather than degrees.
Why?
* There is a subset of Curv that needs to be compiled into GLSL
  for direct execution on a GPU, and there's a benefit if the trig functions
  match the GLSL specification.
* Virtually every other language uses radians as input to trig functions.
  This makes radians better if you are porting geometric code from another
  language.
* Radians are more natural for geometric algorithms, since radians correlate
  the distance around the perimeter of a circular wedge with the angle subsumed.

standard trig constants:
* `e = 2.718281828459045;`
* `pi = 3.14159265358979323846;`
* `tau = pi * 2;`
* `deg = tau / 360;`

Degrees are the familiar unit for specifying angles in calls to rotate, etc.
You can write `rotate(90*deg)`.
This is equivalent to `rotate(tau/4)` (where `tau` is a full turn).

## Sequences
Here are some operations that are generic over strings, lists and objects.

```
len(X) -- # of elements in the sequence
X@i -- i'th element of X
X@(i..j) X@(i..) X@(..j) -- slice notation
X@(i:step..j) X@(i:step..) X@(:step..j) -- slice notation with stride (alt 1)
  This is very close to the Matlab/Julia/OpenSCAD range syntax of
  start:step:end, but less confusing/ambiguous for Python programmers.
X@list -- sequence containing all elements whose indices are in list
  Note that slice notation above doesn't support a stride,
  but you can still write X@[0..len(X)-1:2].
reverse(sequence)
join(separator-sequence)(list-of-sequences)
  eg, join "," ["foo","bar","baz"] -> "foo,bar,baz"
sort(sequence,compare=(x,y)->x<=y)
```

F# uses `X.[i]` to index into a sequence.
By extension, I could also use `X.{key}` to index into a map (record or module),
where `key` is a string. This is elegant and consistent, at least.
```
[1,2].[0] == 1
{x=1,y=2}.{"x"} == 1
```

Strings are not lists of characters, and not all sequence operations work
on strings. Here are some that only work on lists and objects.
* concat(list-of-sequences)
  * `concat[]` returns `[]`, not `""`, so `concat` is not appropriate for
    string concatenation. Maybe use `strcat`.
* for (i = X) ... -- iterate over elements of X in a list/object literal
  * `[for(c=str)c]` does not return `str`.
    But you can use `strcat[for(c=str)c]`.
    If this were an important use case, I could provide `strfor`,
    which strcats each element together. Or, "?for(i=s)...;",
    since string literals are much like list comprehensions anyway.

Consider if sequences were indexed using funcall notation.
More expressive: polymorphism between sequences/functions,
ability to use the full range of function call syntax for indexing.
* How would slice notation work?
* This interferes with the OpenSCAD2 design for objects, where customization
  is funcall, as distinct from indexing.
* just use i->a@i to convert sequence to function

`map(f)(seq)`
> When applied to a list,
> `map(x->x+1) L` is like `[for (x=L) x+1]`,
> except that, syntactically, the list expression L is written at the end,
> which may be useful for writing pipelines.
> `map` preserves the stringiness of a string and the metadata of an object.

`filter(f)(seq)`
> When applied to a list,
> `filter(x->x!=0) L` is like `[for (x=L) if (x!=0) x]`,
> except that it moves the list expression L to the left.
> `filter` preserves the stringiness of a string and the metadata of an object.

## Strings
A string is a sequence of 0 or more unicode characters.
The two most important features are string literals, and concatenation.

Concatenation: `strcat[s1,s2,...]`

### Escape sequences
Which escape character should we use in string literals?

Here are the important use cases for string literals:
* file names (of local resources).
* urls (of remote resources).
* debug messages (via echo and assert).
* multi-line documentation strings, using some markup language
  (MarkDown and ReStructured Text are the most popular).

OpenSCAD uses `\` as the escape sequence.
It's not the best choice:
* On Windows, `\` is the path separator. This causes confusion for beginners,
  which is best avoided.
* `\` is also the escape character for MarkDown and ReStructured Text.

For writing debug messages, I'd like the ability to interpolate variables,
using `$X` like in the Unix shell.
The `$` character is a pretty good choice as an escape character,
since it doesn't conflict with filenames, urls or markup languages.

Escape sequences:
* `$`*identifier* -- interpolate as expression, common case for debugging
* `$(`*expression*`)` -- interpolate as expression
* `${`*expression*`}` -- interpolate strings literally, rest as expression
* `$+`*hexdigits* or `$+{`*hexdigits*`}`. Unicode code point.
  Eg, U+1F600 (smiley face) is written as `"$+1F600"`.

For example, `echo("x=$x, y=$y")`.

OpenSCAD defines `\t`, `\r` and `\n` as names for special characters.
We don't need this syntax, since character names can be defined by libraries.
The standard library will define `nl="$+A";`, so you can use "${nl}"`
to interpolate a newline. (Braces are needed for literal string interpolation.)
But these special cases are worth having:
* `$$` --  literal `$`
* `$"` --  literal `"`

Note that a special variable like `$t` must be interpolated
as `"${$t}"` or `"$($t)"`. (This seems bad, but at this point I might not
implement special `$` variables.) Ruby uses `#{expr}` for interpolation.

Note that, because arbitrary expressions (not just variable names)
can be interpolated into string literals, that could mean there's a
recursive dependency between the lexical analyzer and the parser.
Or, the lexer keeps track of nesting levels for ()[]{}"" while
tokenizing the subexpression.

### String literals:

`"`...`"` is a string literal which must begin and end on the same line.

A Python-style multi-line text literal begins and ends with `"""`.
These are used for debug messages and doc strings.

Both types of string literal may contain escape sequences beginning with `$`.

It bugs me that Python multi-line literals can't be indented.
Here's another syntax for multi-line string literals:
```
echo "first line
     >second line
     >third line";
```
PS the python solution is "implicit string concatenation",
```
echo "first line\n"
     "second line\n"
     "third line";
```

### Printing a String
How are strings printed, using `$(str)` interpolation? As a quoted string,
but how are newlines printed? Note that there is no built-in `\n` escape,
but `${nl}` works.

The simplest design is to escape `$` and `"` and print everything else
literally. `"`...`"` string literals may span multiple lines.

### Code Points vs Characters

The Unicode standard doesn't define the term "character",
but instead defines "code point" and "grapheme cluster".
Curv provides access to the individual characters within a string,
and it considers a "character" to be a grapheme cluster. This is because
when the `text` primitive renders a string, each "character" that we see
in the output is actually a grapheme cluster. For example, an accented
Latin character like é is represented by multiple code points
(in the general case), one for the base character and one for each accent,
although for the most
common cases, a single code point is also available as an alias.
* The concatenation two strings of length M and length N
  should have length M+N. So a string can't begin with
  a code point that only appears in the middle or end of a character.
  This is enforced in string literals and by `code_to_str`
  by checking the first code point.

To obtain lower level access to the Unicode string representation,
`str_to_code` converts a string to a list of integers (each integer is
a unicode code point), while `code_to_str` is the reverse transformation.
If you need direct access to the individual code points within a string,
as opposed to just accessing the characters (grapheme clusters),
then you use these functions.

In Curv, if two grapheme clusters are guaranteed to have the same
printed representation, then they compare equal.
In the OpenSCAD2 design, I argue that strings should behave like lists
of characters. Eg, you should be able to iterate over the characters in a string
using a `for` loop. But for this to work perfectly, we'd need a character
data type. Not sure about this yet.

## Lists

### List Constructors
- list literals and comprehensions, as in OpenSCAD
- range literals are lists, as in OpenSCAD2

Range literal syntax:
* [first:step:last] is from Matlab. A problem arises when we extend this
  syntax to slice notation: v@(first:step:last) is too confusible with
  the conflicting Python slice notation of first:last:step.
* `first..last` is a widely used alternative for ranges of step 1.
  It has excellent readability, since the use of ellipsis
  conforms with standard math notation.
* `[first,next..last]` is Haskell notation for a range with step (next-first).
  It conforms to standard math notation.
  However, it conflicts with the extended list comprehension syntax
  of [generator,generator].
  It also conflicts with multi-dimensional array slices, since in that context
  the `,` separates the slice specifiers for each axis.
* `[first..last:step]` is a compromise. It can't be misunderstood by either
  Python or Matlab users. It can be used as part of an extended list
  comprehension and array slice notation.

Within a list constructor, `include list_expr` interpolates the elements
of `list_expr` into the list being constructed.
(So, I'm not using `each` or `&`.)

### Lazy Lists
I'd like to support lazy lists (see Why Functional Programming Matters).
I have an implementation in mind that would support lazy infinite lists like:
```
ones = cons(1, ones);
```

Lazy linked lists and strict arrays have the same data type:
there are just a multiple internal representations for the list type.

These builtin functions provide the abstract interface for Lisp/Haskell style
list algorithms:
```
empty(v) = len(v) == 0;
first(v) = v@0;
rest(v) = v@(1..);
cons(e,v) = concat[[e],v];
```

The syntax `[a,b,c]` constructs a strict array with O(1) indexing.
The syntax `cons(e,v)` constructs a cons cell, part of a linked list
which is eventually terminated by an array.
The recursive definition
```
ones = cons(1, ones);
```
causes `ones` to be implemented as a thunk, due to the fact that the compiler
has detected recursion. This results in a lazy list, not because `cons`
is necessarily lazy, but because `ones` is lazy.

There is an `array` function which flattens a linked list to an array.
This doesn't change its type, just the internal representation, copying the data
if necessary in order to ensure that the result has O(1) indexing.
* This is less abstract than providing a self-adjusting list data structure
  that automatically reshapes itself to provide good performance for all of
  the operations (such as the various tree-based "functional array" data
  structures). But, the reason there are so many functional array data
  structures is that they have different performance tradeoffs, and you need
  to make a choice based on your performance requirements. If you are tuning
  the performance of a Curv program, then `array` may give you the ability
  to speed things up by giving you more control over the representation.
* It's more abstract than the common approach of providing multiple
  incompatible 'sequence' data types (eg, list vs array) and forcing the user
  to choose one.

List comprehensions are lazy (consistent with F# seq constructors).
Or maybe, [for (i=a) ...] returns an array if a is an array, or returns
a lazy list if a is a lazy list. It might also depend on the size of a.

What is the performance of `concat`? Copy everything and construct a flat
array? Or is it allowed to be faster and return some kind of tree?
* What if an infinite list is given as an argument?
  It should be possible to prepend elements to a lazy list, and you can
  using `cons`, but shouldn't this work as well?
  We can mark a lazy list as having unknown length and `concat` can test this.
* Maybe this: `concat[a,b,c]` == `cat(a,cat(b,c))` where `cat` is another node
  type.

What is the implementation of a general list comprehension?
* With no generators, [a,b,c] is an array.
* More generally, we process action expressions from right to left:
  * start with result = []
  * a series of expressions is converted to an array a,
    then result = cat(a, result).
  * if (c) expr -> if (c) result := cons(expr, result)
  * for -> result := cat(for-object, result)

### List Library
I want a better library of standard list functions.

`transpose(L)`
> The `L` must be a list of lists, where each sublist has the same length,
> otherwise abort.
> Return the transpose of L: a list of items, where the initial elements
> of each item consist of the elements from the first sublist, and so on.
> Use cases:
> * If L is a matrix (each sublist is a vector), then `transpose L`
>   is the matrix transposition.

`lookup(key, assoc_list)`
> The `assoc_list` is a list of `[key,value]` pairs.
> Find the first pair with the specified key, and return the associated value.
> If the key is not found, return `null`.
> (Name conflicts with OpenSCAD integer lookup table function: use `find`?
> or `search`?) Called lookup in Haskell, member in Scheme, assoc in Lisp.

`flatten(L)`
> Flatten a nested list of arbitrary depth to a flat list of non-list elements.

ideas from Haskell Data.List
```
Basics:
  I considered infix append, using ++ from Haskell, but abandoned it.
  Instead, concat for lists and strcat for strings.

  head, last, tail, init -- Python does this with slices
  length (or len in OpenSCAD)
Transformations:
  map(f)(list) -- like [for (i : list) f i]
  reverse -- Python does this with slices, but that's pretty obscure
  intersperse(item)(list) -- not seen this before
  intercalate(ilist)(list-of-lists) -- seen this called "join", as string op
  transpose(list-of-lists) -- useful: like zip, also for linear algebra
  subsequences(list), permutations(list) -- ? expensive if not lazy
Folds:
  foldl, foldr, and variants
Special folds:
  concat
  and,or -- same as my all,any
  any(p)(list), all(p)(list) -- do it with a predicate
  sum, product, maximum, minimum
Building lists: scans:
  scanl, scanr: like foldl, foldr but returns list of successive reduced values
accumulating maps:
  mapAccumL, mapAccumR : abstract, man
Infinite lists:
  iterate, repeat, replicate, cycle -- does Curv need lazy lists?
Unfolding:
  unfoldr -- I prefer my 'for(;;)' syntax
Extracting Sublists:
  take(n)(list), drop(n)(list) -- Python uses slices
  splitAt(n)(list)
  takeWhile(f)(list), dropWhile(f)(list)
  span(f)(list), break(f)(list)
  stripPrefix(prefix)(list)
  group(list) -- partition list into fragments where each element is equal
  inits(list)/tails(list) -- list of initial/final subsequences
Predicates:
  isPrefixOf(prefix)(list)/isSuffixOf(suffix)(list)
  isInfixOf(sublist)(list)
  -- aka begins_with, ends_with, contains
Searching lists:
..by equality:
  elem(e)(list) -> Bool
  notElem(e)(list) -> Bool
  lookup(key)(association-list) -- replacement for 'search'
..with a predicate:
  find(p)(list) -- leftmost element matching p, or null
  filter(p)(list)
  partition(p)(list)
Indexing lists:
  list!!index
  elemIndex(elem)(list) -> index of first matching element, or null
  elemIndices(elem)(list) -> list of indices
  findIndex(p)(list)
  findIndices(p)(list)
Zipping and Unzipping
  zip...zip7 could be unified as transpose
  zipWith..zipWith7 -- parallel for in list comprehension?
  unzip..unzip7 -- transpose again
String ops
  (line and word parsing)
Set operations
  representing a mathematical set using a list
Ordered lists
  sort
```
From Python and array languages, there is the idea of slice notation,
and multi-dimensional slices. list@(slice1,slice2) where 'slice' is a value
that selects either a specific item, or a list of items matching some criterion.
Could be a DSL for specifying slices. Or regexes. Or qed addresses. Gawd.

## Objects
Like OpenSCAD2.

The syntax of scripts and object literals is simplified
by replacing statements with expressions.
A script is a sequence of expressions, generators and definitions,
separated by `;`, with an optional trailing `;`.

Within a script or object literal, definitions are lazily evaluated in
dependency order, rather than eagerly evaluated in source code order.
The order in which you write definitions does not matter.
Definitions are eagerly parsed to an AST, but they are lazily compiled
to VM code at evaluation time (so there is a simple JIT compiler).
So it's not too expensive to reference a large library where most of the
definitions aren't used.

## Data Types and Pattern Matching
Curv is a dynamically typed language.
A "type" is a special kind of subset of the set of all values:
it's a subset that specifies the domain or range of a function,
or the domain of a user-modifiable script variable.

I'm thinking about a little language for expressing commonly used types.
We'll call these "type expressions".
Here are the use cases:
* Testing the type of a value. This is currently done in OpenSCAD by invoking
  an illegal operation on the value, and testing for a result of `undef`,
  but that won't work in Curv, since we abort on a type error.
* For convenience, a pattern-matching switch statement.
* Annotate the formal parameters of a function with their types,
  so that we can perform a run-time check and abort a function call
  if any of the arguments have the wrong types. This addresses an ease-of-use
  issue in Curv. The built-in functions all do this (check their arguments).
  If you can't do it for user-defined functions, then a user defined function
  might abort deep in the middle of a function call, and interpreting the
  error message may require understanding the code.
* Annotate the domain of a script variable, for use by the Customizer GUI.

A good place to begin is by looking at all of the built-in operations,
and giving names to the most useful domain and range types.
For example,
* Boolean
* Number
* Integer
* String
* List(type=Any, len=infinity)
  Eg, `List()` means any list, `List(Number)` is a list of numbers (vector),
  `List(Number,3)` is a 3-vector.
* Tensor
* Sequence (domain of `len`)

Another place to look is the type annotations
used by the Thingiverse and proposed OpenSCAD customizer GUI.
* Continuous numeric range, for a slider gui.
  Eg, `0...1`
* Discrete list of values, usually strings or numbers, for a drop-down menu.
  Eg, `a|b|c`

Syntax:
* *value* `isa` *type*
* `switch(`*value*`) case` *pattern1* `->` *result1* ...
* `f(x : type) = ...`
* `myvar : type = ...`

A *pattern* is a kind of expression that matches a value against a type,
and binds zero or more variable names to values derived during the pattern
match. A pattern is used on the left side of a definition,
as a formal parameter in a function, as a pattern in a `case` expression.
This is standard stuff, found in all mainstream functional languages.

Patterns:
* identifier
* identifier [`:` type] [`=` defaultvalue]
* `[]`, `[pattern]`, `[pattern1,pattern2]`, ...
* `_` -- a special placeholder identifier that isn't bound, the matched
  value is just discarded.
* (expr) -- matches any value == (expr). String and numeric literals don't
  need to be parenthesized.

Type values could be represented as predicate functions.
* Eg, is_number(X) instead of X isa Number.
* Then we have predicate operations: ~P, P1|P2, P1&P2.
  Eg, filter(is_list|is_number)
* More predicates: ==X, !=X, >X, <X, >=X, <=X.
  * Eg, filter(is_integer & >=0)
  * No need for value equality patterns. Eg, case: ==12 =>
* cases in a switch can use any predicate.
* 'type declarations' in a function formal parameter list
  can use any predicate.
  * succ(x:is_number) = x + 1;

I've proposed 'varieties' as a mechanism for user-defined object types.
Syntax:
* new_variety(V1,V2,...) -- construct a new variety which inherits from
  zero or more parents. Has a compile time side effect, so that two distinct
  calls to new_variety() at different locations within the source code
  return different values.
* `isa <variety>;` as a statement within a script.
* `X isa <variety>`

I also proposed to have built-in varieties like Number, but you can't
use built-in varieties with inheritance or an isa statement.
So, in that scheme, there are two kinds of variety.
* Alternatively, built-in varieties become ordinary predicates,
  and user-defined varieties become a subtype of predicate.

Rename variety to 'contract'.
* C=new_contract(C1,C2,...); -or- contract C(C1,C2,...);
* implements <contract>;
