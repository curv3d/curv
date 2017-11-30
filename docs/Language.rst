=================
The Curv Language
=================

Curv is a domain specific language for specifying geometric objects.
It's a language for artists, and it prioritizes simplicity and exploratory
programming over the complex features provided by general purpose programming languages
to support large scale software engineering.

Curv is a dynamically typed pure functional language.
It has simpler semantics than any general purpose programming language.
It's powerful enough that the entire geometry API is written in Curv, not C++.
The core language is small enough to learn in a day.

1. Overview
===========

Values
------
In Curv, everything is a value.
All values are immutable (there are no mutable objects).
All data structures are hierarchical and finite, and therefore printable.
This design make Curv programs easier to understand and debug.

Curv has 7 primitive value types:

==============     ============================================
null               ``null``                
boolean            ``true, false``
number             ``-1, 3.1416, 0xFF``
string             ``"hello, world"``
list               ``[0,1,2]``
record             ``{name: "Lancelot", quest: "To seek the holy grail", favourite_colour: "blue"}``
function           ``x -> x+1``
==============     ============================================

The first 6 data types are taken from JSON,
and Curv can be considered a superset of JSON.

There are no type declarations or class declarations.
All application data is represented using the 7 value types.
For example, a geometric point or vector is a list of numbers.
A matrix is a list of lists of numbers. A geometric shape is a record
containing 5 fields (two booleans, two functions and a matrix).

Program Structure
-----------------
Curv has no I/O: it is not a scripting language.
The only goal of a Curv program is to compute a value:
usually this is a geometric shape, but it can also be a library.
(Shapes and libraries are both represented by record values.)

Conventional programming languages are "statement languages". A statement
is a syntactic form that either defines a named object (a definition)
or causes a side effect (an action). In such a language, a program is a sequence
of statements, the body of a function is a sequence of statements (one of which
is a "return statement"), and expressions occur only as elements of statements.

Curv is an "expression language". An expression is a syntactic form that
computes a value, eg like ``2+2``. A program is an expression. The body of a function
is an expression. Statements only occur embedded in certain expression forms.
(Note that JSON is also an expression language, and that most JSON programs are
also legal Curv programs.)

Blocks
------

Modules
-------

Functions
---------

List, Record and String Comprehensions
--------------------------------------

Design by Contract
------------------

Arrays and Vectorized Operations
--------------------------------

2. Values and Operations
========================

Null
----
``null`` is a placeholder value that, by convention, indicates that
some situation does not hold, and that the corresponding value is not available.
Its distinguishing characteristic
is that it is different from any other value. The only available
operation is testing a value to see if it is null: ``value==null``
is true if the value is null, otherwise false.

Boolean Values
--------------
The Boolean values are ``true`` and ``false``.
They are used for making decisions:
if some condition holds, do this, otherwise do that.

The relational operators compare two values and return a boolean:

==============     ============================================
``a == b``         ``a`` is equal to ``b``
``a != b``         ``a`` is not equal to ``b``
``m < n``          ``m`` is less than ``n``
``m <= n``         ``m`` is less than or equal to ``n``
``m > n``          ``m`` is greater than ``n``
``m >= n``         ``m`` is greater than or equal to ``n``
==============     ============================================

(Note: ``a`` and ``b`` are arbitrary values, ``m`` and ``n`` are numbers.)

The ``==`` operator is an equivalence relation:

* For every value ``a``, ``a == a``.
* For any pair of values ``a`` and ``b``, if ``a==b`` then ``b==a``.
* For any three values ``a``, ``b`` and ``c``, if ``a==b`` and ``b==c`` then ``a==c``.

The logical operators take boolean values as arguments, and return a boolean:

==========   =============================================================
``a && b``   Logical and: True if ``a`` and ``b`` are both true.
``a || b``   Logical or: True if at least one of ``a`` and ``b`` are true.
``!a``       Logical not: ``!true==false`` and ``!false==true``.
==========   =============================================================

The conditional operator selects between two alternatives based on a boolean condition::

  if (condition) result_if_true else result_if_false

For some algorithms, it is convenient to represent booleans as integers:
``true`` is ``1`` and ``false`` is ``0``. We support this via conversions
between boolean and integer:

* ``bit b`` -- convert boolean value ``b`` to an integer
* ``i != 0`` -- convert an integer bit value ``i`` to a boolean

Numbers
-------
Numbers are represented internally by 64 bit IEEE floating point numbers.

* ``0`` and ``-0`` are different numbers, but they compare equal.
* ``inf`` is infinity; it is greater than any finite number.
* ``-inf`` is negative infinity.
* We don't support the IEEE NaN (not a number) value.
  Instead, all arithmetic operations with an undefined result report an error.
  For example, ``0/0`` is an error.
* An integer is just a number whose fractional part is ``0``.
  ``1`` and ``1.0`` evaluate to the same integer.

The arithmetic operators:

=========  ==============
``-m``     negation
``+m``     identity
``m + n``  addition
``m - n``  subtraction
``m * n``  multiplication
``m / n``  division
``m ^ n``  exponentiation
=========  ==============

``abs n``
  The absolute value of *n*.

``max list``
  The maximum value in a list of numbers.
  ``max[]`` is ``-inf``, which is the identity element for the maximum operation.

``min list``
  The minimum value in a list of numbers.
  ``min[]`` is ``inf``, which is the identity element for the minimum operation.

``floor n``
  The largest integer less than or equal to *n*.

``ceil n``
  The smallest integer greater than or equal to *n* (ceiling).

``trunc n``
  The integer nearest to but no larger in magnitude than *n* (truncate).

``round n``
  The integer nearest to *n*. In case of a tie (the fractional part of *n* is 0.5),
  then the result is the nearest even integer.

``mod(a,m)``
  The remainder after dividing ``a`` by ``m``,
  where the result has the same sign as ``m``.
  Equivalent to ``a - m * floor(a/m)``.
  Aborts if ``m==0``.

``rem(a,m)``
  The remainder after dividing ``a`` by ``m``,
  where the result has the same sign as ``a``.
  Equivalent to ``a - m * trunc(a/m)``.
  Aborts if ``m==0``.

``phi``
  The golden ratio: ``(sqrt 5 + 1) / 2 == 1.618033988749895``.

``e``
  Euler's number: ``2.718281828459045``.

``sqrt n``
  Square root of *n*.

``log n``
  Natural logarithm (to the base *e*) of *n*.

``clamp(n,lo,hi)``
  Constrain ``n`` to lie between ``lo`` and ``hi``.
  Equivalent to ``min(max(n,lo),hi)``.

``lerp(lo,hi,t)``
  Linear interpolation between ``lo`` and ``hi``
  using parameter ``t`` as a weight: ``t==0`` returns ``lo``
  and ``t==1`` returns ``hi``.
  Equivalent to ``lo*(1-t)+hi*t``.

``smoothstep(lo,hi,x)``
  Return 0 if x <= lo; 1 if x >= hi;
  otherwise smoothly interpolate between 0 and 1 using a Hermite polynomial.
  Result is undefined if lo >= hi.
  https://en.wikipedia.org/wiki/Smoothstep

Trigonometry
------------
Curv uses `radians`_ (not degrees) to specify angles.

.. _`radians`: https://en.wikipedia.org/wiki/Radian

``pi``
  The ratio of a circle's circumference to its diameter: ``3.141592653589793``.

``tau``
  The number of radians in a full turn, aka ``2*pi`` or ``360*deg``.

``deg``
  The number of radians in an angle of 1 degree.
  To specify the angle "45 degrees", use ``45*deg``.

``sin x``
  The sine of ``x``, measured in radians.

``cos x``
  The cosine of ``x``, measured in radians.

``tan x``
  The tangent of ``x``, measured in radians.

``asin x``
  The principal value of the arc sine (inverse sine) of x.
  The result is in the range [-pi/2, +pi/2].

``acos x``
  The principle value of the arc cosine (inverse cosine) of x.
  The result is in the range [0, pi].

``atan x``
  The principal value of the arc tangent (inverse tangent) of x.
  The result is in the range [-pi/2, +pi/2].
  The ``atan2`` function is generally more useful.

``atan2(y,x)``
  The principal value of the arc tangent of y/x,
  using the signs of both arguments to determine the quadrant of the return value.
  The result is in the range [-pi, +pi].
  
  Used mostly to convert from rectangular to polar coordinates,
  but for that, you should consider using ``phase(x,y)`` instead,
  so that you don't have to flip the x,y coordinates.

``sec x``
  The secant (reciprocal sine) of x.

``csc x``
  The cosecant (reciprocal cosine) of x.

``cot a``
  The cotangent (reciprocal tangent) of x.

Lists
-----
A list is a finite, ordered sequence of values.

Simple list constructor
  * A sequence of *n* comma separated expressions, enclosed in square brackets,
    constructs a list with *n* values. The final expression may have an optional
    trailing comma.
    Eg: ``[]``, ``[a]``, ``[a,b]``, ``[a,b,]``...
  * As an alternate syntax, you can also use parentheses to enclose the list,
    with the caveat that the list constructor must either be empty, or must contain
    commas. ``(expression)`` is just a parenthesized expression, it's not a 1-element list.

List comprehension

Range constructor

List indexing

::

  count
  concat vv = [for (v in vv) for (i in v) i];
  reverse v = v[count(v)-1..0 by -1];
  map f list = [for (x in list) f x];
  filter p list = [for (x in list) if (p x) x];
  reduce (zero, f) list =
      if (list == [])
          zero
      else
          do  var r := list[0];
              for (i in 1..<count list)
                  r := f(r, list[i]);
          in r;
  sum = reduce(0, (x,y)->x+y);
  product = reduce(1, (x,y)->x*y);

Array
-----
::

  // complex numbers: [RE,IM]
  RE=0;
  IM=1;
  cmul(z,w) = [z[RE]*w[RE] - z[IM]*w[IM], z[IM]*w[RE] + z[RE]*w[IM]];
  csqr(z) = [ z[RE]*z[RE] - z[IM]*z[IM], 2*z[RE]*z[IM] ];

  ////////////////////
  // Linear Algebra //
  ////////////////////
  X = 0;
  Y = 1;
  Z = 2;
  is_vec2 x = is_list x && count x == 2 && is_num(x[0]) && is_num(x[1]);
  is_vec3 x = is_list x && count x == 3 && is_num(x[0]) && is_num(x[1]) && is_num(x[2]);
  cross(p,q) = [p[Y]*q[Z] - p[Z]*q[Y], p[Z]*q[X] - p[X]*q[Z], p[X]*q[Y] - p[Y]*q[X]];
  identity(n) = [for(i in 1..n) [for(j in 1..n) if(i==j) 1 else 0]];
  transpose(a) = [for (i in indices(a[0])) [for (j in indices a) a[j,i]]];
  normalize v = v / mag v;

  // phase angle of a vector, range tau/2 to -tau/2
  phase v = atan2(v[Y],v[X]);

  // convert phase angle to unit vector
  cis theta = [cos theta, sin theta];

  // perp: Rotate a 2D point by 90 degrees CCW. Multiply a complex number by i.
  // It's the 2D analog of the 3D vector cross product (Cross in Mathematica).
  // dot(perp a, b) is the "perp-dot" product:
  // see: 'The Pleasures of "Perp-Dot" Products', Graphics Gems IV.
  perp(x,y) = (-y, x);

  // angle between two vectors
  angle(a,b) = acos(dot(a,b) / (mag a * mag b));

  dot
  mag

String
------
::

  nl = decode[0xA]; // ASCII newline
  strcat
  repr
  decode
  encode
  count

Record
------
::

  merge rs = {for (r in rs) ...r};
  fields
  defined

Function
--------
::

  switch

Actions
-------
::

  print
  warning
  error
  assert
  assert_error
  exec

Source Files and External Libraries
-----------------------------------
::

  file
  use record_expr

Design by Contract
------------------
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

3. Grammar
==========

The Parse Tree
--------------
The following abstract grammar has just enough structure to parse
a source file into an abstract parse tree. It shows that there are 
12 operator precedence levels, with ``list`` being the lowest precedence
and ``postfix`` being the highest precedence::

  program ::= list

  list ::= empty | item | commas | semicolons | item 'where' list
    commas ::= item ',' | item ',' item | item ',' commas
    semicolons ::= optitem | semicolons `;` optitem
    optitem ::= empty | item

  item ::= pipeline
    | '...' item
    | 'use' item
    | pipeline '=' item
    | pipeline ':=' item
    | pipeline ':'
    | pipeline ':' item
    | pipeline '->' item
    | pipeline '<<' item
    | 'if' parens item
    | 'if' parens item 'else' item
    | 'for' '(' item 'in' item ')' item
    | 'while' parens item
    | 'let' list 'in' item
    | 'do' list 'in' item

  pipeline ::= disjunction
    | pipeline '>>' disjunction
    | pipeline '`' postfix '`' disjunction

  disjunction ::= conjunction | disjunction '||' conjunction

  conjunction ::= relation | conjunction && relation

  relation ::= range
    | range '==' range | range '!=' range
    | range '<' range  | range '>' range
    | range '<=' range | range '>=' range

  range ::= sum
    | sum '..' sum
    | sum '..' sum 'by' sum
    | sum '..<' sum
    | sum '..<' sum 'by' sum

  sum ::= product | sum '+' product | sum '-' product

  product ::= unary | product '*' unary | product '/' unary

  unary ::= power | '-' unary | '+' unary | '!' unary | 'var' unary

  power ::= postfix | postfix '^' unary

  postfix ::= primary
    | postfix primary
    | postfix '.' primary

  primary ::= identifier | numeral | string | parens | brackets | braces
    identifier ::= /[a-zA-Z_] [a-zA-Z_0-9]*/, except for reserved words
      reserved_word ::= '_' | 'by' | 'do' | 'else' | 'for' | 'if'
        | 'in' | 'let' | 'use' | 'var' | 'where' | 'while'

    numeral ::= hexnum | mantissa | /mantissa [eE] [+-]? digits/
      mantissa ::= /digits/ | /'.' digits/ | /digits '.'/ | /digits '.' digits/
      digits ::= /[0-9]+/
      hexnum ::= /'0x' [0-9a-fA-F]+/

    string ::= /'"' segment* '"'/
      segment ::= /[white space or printable ASCII character, except for " or $]+/
        | /'""'/
        | /'$$'/
        | /'${' list '}'/
        | /'$[' list ']'/
        | /'$(' list ')'/
        | /'$' identifier/

    parens ::= '(' list ')'
    brackets ::= '[' list ']'
    braces ::= '{' list '}'

  C style comments, either '//' to end of line, or '/*'...'*/'

Phrases
-------
There is a deeper phrase-structure grammar that assigns syntactic meanings
to most parse tree nodes, which are now called phrases.
(Some parse tree nodes do not have an independent meaning, and are not phrases.)
There are 6 phrase types:

definition
  A phrase that binds zero or more names to values, within a scope.

pattern
  A pattern can occur as a function formal parameter,
  or as the left side of a definition, and contains usually one
  (but generally zero or more) parameter names.
  During pattern matching,
  we attempt to match an argument value against a pattern.
  If the match is successful, we bind (each) parameter name
  to (elements of) the argument value.

expression
  A phrase that computes a value.

action
  A phrase that causes a side effect, and doesn't compute a value.

element generator
  A phrase that computes a sequence of zero or more values.
  ``[``\ *element_generator*\ ``]`` is a list comprehension.

field generator
  A phrase that computes a sequence of zero or more fields,
  which are name/value or string/value pairs.
  ``{``\ *field_generator*\ ``}`` is a record comprehension.

An action can be used in any context requiring a definition,
element generator, or field generator. An expression can be used
in any context requiring a field generator.

Programs
--------
There are two kinds of programs.
A source file is always interpreted as an expression.
A command line (in the ``curv`` command line interpreter)
can be an expression, an action, or a definition.

Phrase Abstraction
------------------
Curv has a set of generic operations for constructing more complex phrases
out of simpler phrases. These operations work on multiple phrase types,
and support sequencing, conditional evaluation, iteration, and local variables.

Parenthesized phrase: ``(phrase)``
  Any phrase can be wrapped in parentheses without changing its meaning.

Compound phrase: ``phrase1; phrase2``
  * If both phrases are definitions, then this is a compound definition.
    The order doesn't matter, and the definitions may be mutually recursive.
  * If both phrases are actions, element generators, or field generators,
    then the phrases are executed in sequence.

Single-arm conditional: ``if (condition) phrase``
  If the phrase is an action, element generator, or field generator,
  then the phrase is only executed if the condition is true.

Double-arm conditional: ``if (condition) phrase1 else phrase2``
  The phrases may be expressions, actions, element generators, or field generators.

Bounded iteration: ``for (pattern in list_expression) phrase``
  The phrase may be an action, element generator, or field generator.
  The phrase is executed once for each element in the list.
  At each iteration,
  the element is bound to zero or more local variables by the pattern.

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase can be an expression, action, element generator or field generator.

Local variables: ``phrase where definition``
  An alternate syntax for defining local variables.

Local actions: ``do action in phrase``
  The phrase can be an expression, action, element generator or field generator.
  The action is executed first, then the phrase is evaluated.

4. The Imperative Sublanguage
=============================
Curv contains an "imperative sublanguage", implemented by the ``do`` operator,
which allows you to define mutable variables, and iterate using
a ``while`` action. This allows you to write code in an imperative style.
The semantics of this feature are restricted, so that it is impossible to define
impure functions. Curv retains its pure functional semantics.

This feature exists for 3 reasons:

* Makes it easier to port code from an imperative language.
* It's an aid to users whose primary programming experience
  is with imperative languages, and who have not yet learned how to program
  in the functional style.
* In the 0.0 release, this is the only way to iterate within a shape's distance
  function. The GPU compiler is not yet smart enough to convert tail recursion
  into iteration.
