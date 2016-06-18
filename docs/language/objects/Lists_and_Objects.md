* A list is an ordered sequence of values.
* An object is an ordered sequence of values plus a map from names to values.
* Let's explore how they are connected.

What is the purpose of the object design? Why include sequence values?
* Compatibility with OpenSCAD. An OpenSCAD script is imported as an Object
  (or as a Component in the alternate design). An OpenSCAD script with sequence
  values is a parameterized shape.
* It's *not* supposed to mimic Javascript and LUA objects.
  In those languages, an object is a map with string and integer keys.
  A sparse array is an object with non-contiguous integer keys.
  The syntax obj[key] is used to uniformly look up a member using a key,
  where key is either integer or string.
* The design derives from OpenSCAD2, where an object is a geometric group,
  consisting of a sequence of shapes, plus metadata (key/value pairs).
  It derives from re-interpreting some OpenSCAD syntax, where you see
  a mix of assignment statements and module calls:
  * a compound statement
  * a top level script
* The primary use case is to represent a parameterized geometric object,
  with its parameters and constituent shapes. Most OpenSCAD scripts have
  this structure: some parameter definitions, some module calls.
  It's a key concept in the language. An object reifies this concept as a value.
* A secondary use case is to represent a library.
  The library API is the map. The sequence is often present in OpenSCAD
  to provide an example of how to use the library.
* A secondary use case is a map from names to values. Eg, a set of model
  parameters. Eg, the requirements map in shape3d. For this use case, it's
  not important to have sequence elements.
* In theory, an object could represent a set of positional and labeled
  function call arguments, if I wanted to provide an `apply` operator.
  This is low priority.

There is a list concatenation operator:
* infix: `x ++ y`
* monoid: `concat[x,y]`

There is an object `overlay` operator:
* infix: `base overlay extension`
* generator: `overlay extension;`

(I'm going to explore whether there is enough similarity to unify these
two operators.)

The semantics of `overlay` are like inheritance with override
in an OOP language. The *base* object has an API, which is preserved,
but you can add new things and override existing things.

What does `overlay` do if the objects contain sequence elements?
Consider the use cases:
* Parameterized Shape:
  * The extension could be a set of parameters, and you could be overriding
    parameters to customize the object. More generally, this could be like
    prototype-based OOP programming, where you override functions to customize
    the design.
  * If this is like OOP, then the extension might be designed as a mixin
    that adds additional geometry, in which case the extension sequence is
    appended to the base sequence.
* Library:
  * OOP programming again, you are customizing the library using a mixin.
    I'm not sure that sequence elements in the base make any sense.
* Parameter Set:
  * Neither base nor extension contain sequence elements.

An overlay must preserve sequence elements in the base.
Sequence elements in the extension are less useful.
It might be reasonable to abort if the extension contains sequence elements.

An alternative: the extension sequence is appended to the base sequence.
If we do this, then there is a close relationship between concat and overlay.
For objects with only sequence and no map, overlay *is* concat.
So overlay is a generalization of concat and objects are a generalization
of lists.

So now it might make sense to unify the operators.
Which leads to:
* `x ++ y`.
   * If both are lists, concat.
   * If base is an object, overlay map entries onto base and concat sequence
     elements onto base. Extension can be list or object.
   * If base is a list, extension is an object,
     construct an object whose sequence is concat of base elements then
     extension elements.
   * This is associative but not commutative. (Due to rule 3.)
   * Identity element is `[]`, due to rule 3.
   * So it's a monoid.
* `concat[x,y]` is the monoid version of the operator.
* `++ extension` is a generator, equivalent to `each` in a list constructor
  and `overlay` in a script. And note that `${str}` is the equivalent within
  a string literal.

Note that `++` doesn't work on strings. Mostly because `concat` can't work
on strings. Prefix `++` will bug C programmers who view it as increment.

In a previous design, I wanted to use `&` as infix concat and as a generator.
I still like it; however it conflicts with p&q for predicates.

I'm stuck with the problem that any syntax I choose for infix concat
and prefix interpolate is going to be weird, since it's not a well known design.

HaHa: use `$`.
```
x $ y
$ some_mixin;
"foo${str}bar"
[foo, $ list, bar] 
```
Oops: it sucks that `$list` is interpreted as a special variable.
Could use `%foo` or `@foo` for special variables?
This will be really annoying to conservatives in the OpenSCAD "base".

List/String concatenation in other languages:
* Haskell: x ++ y
* Python: x + y
* Ada, Visual Basic: x & y (for strings)
* Oracle SQL: x || y (strings)
* Math: x || y (mathworld, strings), m1||m2 (wikipedia, matrices)
* Mathematica: x <> y

Use `&` for infix concat and for interpolation in strings, lists, scripts.
```
x & y
& some_mixin;
"foo&{str}bar"
[foo, &list, bar] 
```

The problem with unifying the syntaxes
for string literal interpolation and list/object interpolation
is that this is meant to visually connect to the infix concat operator.
If I make those the same for strings and lists, then it's a problem that
the concat monoid doesn't work on strings.

So it's best to keep these separate. Keep using '$' for string interpolation.

So the list/object concat/interpolate operator need not be a single character.
It could be a symbol or word.
* `&`
* `++`
* `||`
* `et`
* `join`

I need interpolate. I don't need infix concat, since I've got `concat`.
I need an interpolation operator that makes sense for both maps and lists.
* `with`.
  `[a, with b, c]`
  `with b;`
* `include X`. Interesting. How is object interpolation similar/different to
  OpenSCAD include?
  * similar: names from X override earlier names in base.
    sequence values are added to base.
  * different: the base script can't override names introduced early
    by an include.

Okay, I'm liking the `include` operator.
```
include script("foo.tcad");
[a, include b, c]
```
In a script or scoped object literal,
it is *as if* include statements are moved to the end.

The problem with `include` is when you include a list into an object.
You expect the list elements to appear right where the `include` statement is.
* Maybe: we need to distinguish list-include from object-include.
  Despite the fact that both operations are subsumed by `concat`.
* Maybe: that's what happens.
  * The elements appear right where the `include` statement is, when including
    either a list or object. That's consistent with order being important
    for element expressions in a list or object constructor.
  * But when including an object, the fields behave like they appear at the end,
    overriding, but not being overridden. That's consistent with order not
    being important for field definitions in an object literal.
