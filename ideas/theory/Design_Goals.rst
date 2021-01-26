Design Goals for the Curv Language
==================================
Curv is a simple, learnable language for geometric modelling.
This document explains how we unpack the goals of "simplicity"
and "learnability", and use them to design a new programming language.

Simplicity
----------
High Level
  Curv simplifies programming by suppressing a lot of low level detail
  that you need to attend to in mainstream, general purpose languages.

Declarative Semantics
  A declarative language is supposed to be a high level language in which
  you describe the desired end result, instead of a low level language where
  you specify in detail a sequence of steps the machine must perform in order
  to accomplish the goal. This is vague, but we can make it more specific.

  In 20th century graphics languages (like Postscript), you used a procedural
  API to create graphics. You would set the values of global variables, such
  as the current colour, the current line width, and the current transformation
  matrix, and then you would call procedures like 'circle' that would draw
  shapes onto a canvas, based on the current settings of global variables.

  In Curv, *shapes are values*. A Curv program is an *expression* that
  *denotes* the shape that you want to construct. There are shape constructor
  functions, like ``circle`` and ``cube``, which construct primitive shapes.
  And there are shape combinator functions like ``rotate`` and ``intersection``,
  that take shapes as arguments and return new shapes as results. A Curv
  program works by composing these shape operations, or more generally,
  by implementing an algorithm that computes a shape value. Shapes are first
  class values that can be stored in variables or data structures, passed as
  arguments to functions, and returned as results.

  This style of programming has been variously branded as "declarative",
  "functional", and "denotative".

  Conal Elliott: By "denotative", I mean founded on a precise, simple,
  implementation-independent, compositional semantics that exactly specifies
  the meaning of each type and building block. The compositional nature of
  the semantics then determines the meaning of all type-correct combinations
  of the building blocks. Denotative semantics is what enables precise &
  tractable reasoning and thus a foundation for correctness, derivation,
  and optimization.

  Equational reasoning, referential transparency, substitution semantics, etc.

Orthogonality and Composability:
  Curv is comprised of a small number of orthogonal features that can be
  composed together without restriction. This simplifies the language and
  eliminates unnecessary complexity, but can also make the language more
  expressive by eliminating barriers to composition.

  * Unify features that are almost the same, but with unnecessary differences.
  * Split up features that do two things at once into separate features
    that can be composed.
  * Eliminate the need to write "glue" code when composing language elements,
    by using standard interfaces.
  * Remove restrictions on how language elements can be composed.
  * Eliminate syntactic overhead for common compositions.

Local Reasoning
  Curv has simple and clear semantics. There is no "spooky action at
  a distance". Everything in Curv (source code, data, program execution
  dynamics) has a hierarchical structure and compositional semantics.
  You can understand a thing using local reasoning, by understanding
  the meaning of each part in isolation.

  * Imperative control structures conform to the principles of Structured
    Programming. They have a single entrance and exit. There are no gotos.
  * Data is hierarchical. There are no pointers or object references, hence
    there are no cycles.
  * Local variables are mutable, but values and data structures are immutable.
    When you update an element of a data structure using an assignment
    statement, it is only the contents of the local variable that changes.
  * Functions are pure: the result of a function call depends only on the
    argument values. Function calls do not have side effects.
  * Functions can only observe and modify their own mutable local variables.
    There are no mutable global variables. There is no shared mutable state.
  * As a corollary, nested closures capture the *values* of local variables,
    they do not capture *references* to local variables.
  * In an expression like ``a+b``, the order of evaluation of the operands
    doesn't matter. ``a+b`` is always the same as ``b+a``, no exceptions.
    From within the expression language, you cannot cause or observe side
    effects.

Learnability
------------
Curv is a simple, declarative language for geometric modelling. But Curv is
also its own extension language. These two goals are potentially in conflict,
since the extension language requires a certain amount of complexity and
bureaucracy that we don't want to expose in the modelling language.

To resolve the conflict, to make it more learnable and easier to use,
Curv is designed as a tower of increasing larger language subsets.
You don't need to master the entire language to use one of these subsets.
Each level has a principled design with simple semantics,
and is self contained, with no accidental complexity leaking through from
the lower level dialects.

Here are the language subsets, of increasing size and complexity:

1. Declarative Modelling Language
2. Parametric Modelling Language
3. Imperative Modelling Language
4. Extension Language

Level 1: Declarative Modelling Language
---------------------------------------
At this level, Curv is not a programming language, it is a data description
language, analogous to SVG or JSON.

There is a fixed set of types:

* Plain Old Data types (numbers, symbols, characters, lists and records),
  which are similar to the JSON data types, and which are used to
  construct arguments to graphical operations.
* Graphical data types, such as shapes.

There is a fixed set of operations for constructing and transforming
2D and 3D shapes, such as ``circle``, ``cube`` and ``rotate``.

A Curv L1 source file is simply an expression that denotes a value of one
of the fixed types: either a geometric shape, or plain data. In the former
case, Curv is like SVG, and in the latter case, it is like JSON. Nothing more
complicated is needed at L1.

1. Minimal syntax, no bureaucracy

   Here's a comparison of complete, minimal programs written
   in a variety of modelling languages:

   +-------------+---------------------------------------------+
   | Curv        | ``circle 50``                               |
   +-------------+---------------------------------------------+
   | OpenSCAD    | ``circle(50);``                             |
   +-------------+---------------------------------------------+
   | OpenJSCAD   | ::                                          |
   |             |   function main() {                         |
   |             |     return circle(50);                      |
   |             |   }                                         |
   +-------------+---------------------------------------------+
   | SolidPython | ::                                          |
   |             |   from solid import *                       |
   |             |   shape = circle(50)                        |
   |             |   print(scad_render(shape))                 |
   +-------------+---------------------------------------------+
   | SVG         | ::                                          |
   |             |   <svg xmlns="http://www.w3.org/2000/svg">  |
   |             |   <circle r="50"/>                          |
   |             |   </svg>                                    |
   +-------------+---------------------------------------------+

2. Declarative semantics

   Curv has simple, declarative semantics, which makes it easier to learn
   and understand. Shape operators are pure functions with no side effects.
   
   This also makes Curv a safe language, in the sense that you can download
   a Curv shape file and render it, without worrying about malware embedded
   in the code. (This is a concern when using a general purpose language
   like Javascript or Python as a geometric modelling language.)

Level 2: Parametric Modelling Language
--------------------------------------
* Parametric design: Use numeric parameters to generate a shape
  using an algorithm. The parameters can be separated from the algorithm.
* Curv at L2 becomes a simple functional programming language with minimal
  bureaucracy.
* Curv is still an expression language, and programs are still expressions.
* Roughly equivalent to OpenSCAD, which is also a declarative, algorithmic,
  unbureaucratic modelling language.
* A new type is added: functions.
* Plain Old Data is used to describe shape parameters.
  A complex model might need a large POD data structure to describe
  all its parameters.
* For simplicity and generality, the POD types have no restrictions on how
  values can be combined and nested. There are no "type errors" if the
  elements of a list do not all have the same "type", therefore Curv L2
  is dynamically typed. Adding static type checking would add complexity
  and bureaucracy that isn't appropriate at this level.
* Curv L2 is compatible with live coding. This also requires dynamic typing,
  in the sense that type errors in unexecuted code do not crash a live
  coding session.
* Extensibility: You can define new shape operations in terms of existing
  high level shape operations (defining functions), and you can load
  external libraries.

Level 3: Imperative Modelling Language
--------------------------------------
This level adds imperative features: mutable local variables, a statement
language that includes assignments, conditionals and loops.

Why?

* Because everybody knows how to write imperative programs.
  Imperative programming comprises a small vocabulary of easy-to-understand
  operations from which you can implement any algorithm. 
* Not everybody understands functional programming, which is also more
  complicated. To match the small universal vocabulary of imperative
  programming, you need tail recursion for iteration, which is harder to
  understand, and less convenient, since you must define an auxiliary
  function for each loop. Alternatively, tacit programming with combinators
  is higher level and gives much shorter programs, but you have to master
  a larger vocabulary of combinators and idioms.

The imperative language features do not destroy Curv's declarative semantics.
Functions remain pure. Values remain immutable. Expressions remain
referentially transparent.

Curv L3 is imperative, but not object-oriented.
Local variables are mutable, but values are immutable.
State mutation only occurs at the statement level, at the transition from
one statement to the next. Expressions do not have side effects.
All state mutation is expressed using variations of the assignment statement.

1. There are no mutable objects.
   We don't need to distinguish between mutable and immutable object types.
   We don't need mutable and immutable variants of the same abstract data type.
   Eg, in Python, *tuples* and *strings* are immutable but *lists* are
   mutable. In Curv, these 3 Python types are represented by a single type
   of immutable list values.
 
2. There is no aliasing. Two distinct mutable variable names are guaranteed
   to refer to disjoint mutable state.
 
3. There are no functions or methods that mutate objects (as side effects).
   We don't need mutating and copying variants of the same abstract operation.
   Eg, in Python, ``list.sort()`` is a method that sorts a list object
   by mutating it as a side effect. It doesn't return a result.
   By contrast, ``sorted(list)`` is a function that returns a sorted list,
   but doesn't mutate the list object passed as an argument.
   In Curv, we only need a single ``sort`` function:

   * ``sort list`` is an expression that returns a sorted list, with
     no side effects.
   * ``list!sort`` is an assignment statement that sorts a list variable
     in place, with the same efficiency as ``list.sort()`` in Python.

Curv's "functional" approach to mutable state makes imperative code easier
to write and understand. It simplifies the language, reduces the number of
concepts that need to be learned, and reduces the amount of complexity that
developers need to keep in their head while reading and writing imperative code.

Level 4: Extension Language
---------------------------
This level completes the Curv language, with features for implementing
efficient, high level, easy to use library abstractions. This requires some
of the complexity and bureaucracy of software engineering languages, which
was omitted from the higher level dialects.

* GPU programming.
* Array programming (linear algebra and data parallelism).
* Efficient and compactly represented typed data and typed arrays.
* Abstract data types.
  Hide implementation details from library users, providing a high level
  interface to library data. Type directed and algebra directed design.
* Efficiently detect type errors in calls to library functions at the point
  of call, rather than deep in the body of the function (which requires the
  user to decode a stack trace and understand the function implementation).
