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

2. Operations on Values
=======================

Null
----
``null`` is a placeholder value that, by convention, indicates that
some situation does not hold, and that the corresponding value is not available.
Its distinguishing characteristic
is that it is different from any other value. The only available
operation is testing a value to see if it is null: ``value==null``
is true if the value is null, otherwise false.

::

  null

Boolean
-------
::

  false
  true
  bit

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


Number
------
Numbers are represented internally by 64 bit IEEE floating point numbers.

* ``0`` and ``-0`` are different numbers, but they compare equal.
* ``inf`` is infinity; it is greater than any finite number.
* ``-inf`` is negative infinity.
* We don't support the IEEE NaN (not a number) value.
  Instead, all arithmetic operations with an undefined result report an error.
  For example, ``0/0`` is an error.

An integer is just a number with no fractional part.
Some programming languages make a low-level distinction between integers and numbers
with no fractional part, so that `1` is an integer and `1.0` is not an integer,
but not Curv.

::

  pi
  tau
  inf

  deg = tau/360;
  e = 2.71828182845904523536028747135266249775724709369995;
  phi = sqrt 5 * .5 + .5;
  X = 0;
  Y = 1;
  Z = 2;
  T = 3;
  MIN = 0;
  MAX = 1;
  X_axis = [1,0,0];
  Y_axis = [0,1,0];
  Z_axis = [0,0,1];

  ceil n = -floor(-n);
  trunc n = if (n >= 0) floor(n) else ceil(n);
  mod(a,m) = a - m * floor(a/m);
  rem(a,m) = a - m * trunc(a/m);
  mix(a,b,t) = a*(1-t) + b*t;
  // Smooth Hermite interpolation between 0 and 1 when lo < x < hi.
  // Used to construct a threshold function with a smooth transition.
  // Results are undefined if lo >= hi.
  // https://en.wikipedia.org/wiki/Smoothstep
  smoothstep(lo,hi,x) =
      let t = clamp((x - lo)/(hi - lo), 0, 1);
      in t*t*(3 - 2*t);
  clamp(v,lo,hi) = min(max(v,lo),hi);
  isinf x = x == inf || x == -inf;

  tan a = sin a / cos a;
  atan x = atan2(x,1);
  sec a = 1 / sin a;
  csc a = 1 / cos a;
  cot a = cos a / sin a;

  ensure pred expr = do assert(pred expr) in expr;

  // complex numbers: [RE,IM]
  RE=0;
  IM=1;
  cmul(z,w) = [z[RE]*w[RE] - z[IM]*w[IM], z[IM]*w[RE] + z[RE]*w[IM]];
  csqr(z) = [ z[RE]*z[RE] - z[IM]*z[IM], 2*z[RE]*z[IM] ];

  ////////////////////
  // Linear Algebra //
  ////////////////////
  is_vec2 x = is_list x && count x == 2 && is_num(x[0]) && is_num(x[1]);
  is_vec3 x = is_list x && count x == 3 && is_num(x[0]) && is_num(x[1]) && is_num(x[2]);
  indices a = 0..<count a;
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

  sqrt
  log
  abs
  floor
  round
  sin
  asin
  cos
  acos
  atan2
  max
  min
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

List
----
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
file

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

3. Grammar
==========
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
    | pipeline '||' disjunction
    | pipeline '>>' disjunction

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

  unary ::= power | '-' unary | '+' unary | '!' unary

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
