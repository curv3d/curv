Symbols and Variants
====================

Symbols
-------
Symbols are simple, abstract values with no internal structure.
``#foo`` is a symbol; it prints as ``#foo``; it is only equal to itself.

Symbols are used when you need to distinguish between a fixed set
of named alternatives. For example,

* In Curv, the boolean values are ``#true`` and ``#false``.
* When exporting a .X3D mesh file, you use the ``-Ocolour=`` command line
  option to specify whether you want face colouring or vertex colouring.
  These two alternatives are represented by symbols, so the command line
  syntax is ``-Ocolour=#face`` or ``-Ocolour=#vertex``.

By convention, the symbol ``#null`` can be used as the null value.
When exporting Curv data as JSON, ``#null`` is converted to JSON ``null``.

Symbols are used internally to represent field names in records.
The ``fields`` function takes a record as an argument, and returns a list
of symbols.

Variants
--------
Variant types are a tool for organizing and modelling data.
The core idea is that a variant type consists of a fixed set of alternatives.
Each alternative has a name, and optionally a value.
To construct an instance of a variant type,
you specify the name of one of the alternatives,
plus a value, if that alternative carries a value.

Variant types are a feature of modern statically typed programming languages,
where they have many names: discriminated unions, enumerated types,
variant types, and algebraic data types.

In Curv, you don't need to declare a variant type before you can construct
a variant value. Instead, variants are constructed directly:

* A variant with just a name is a symbol. Eg, ``#foo``.
* A variant with a name and a value is a record with a single field.
  Eg, ``{bar: x}``.

For example, Curv represents value pickers internally as variants:

* ``{slider:[low,high]}``
* ``{int_slider:[low,high]}``
* ``#scale_picker``
* ``#checkbox``
* ``#colour_picker``

If one of your alternatives means "do nothing" or "the value is not available",
then by convention, you can use the symbol ``#null`` for that.

Variant constructors are also patterns.
You can use pattern matching and the ``match`` operator to process variant
data. For example::

  match [
    {slider:[low,high]} -> ...;
    {int_slider:[low,high]} -> ...;
    #scale_picker -> ...;
    #checkbox -> ...;
    #colour_picker -> ...;
  ]

Symbol API
----------
* quoted symbols
* symbol patterns
* ``is_symbol``

Variants
--------
Variant types are a feature of statically typed programming languages.
They have many names: discriminated unions, enumerated types, variant types,
and algebraic data types.
The core idea is that a variant type consists of a fixed set of alternatives.
Each alternative has a name, and optionally a value.

A variant (or variant value) is an instance of a variant type.
To construct a variant, you need to specify the name of one of the alternatives,
plus a value, if that alternative carries a value.

Variant types are a useful tool for organizing and modelling data.
In Curv, you don't need to declare a variant type before you can construct
a variant value. Instead, variants are constructed directly:

* A variant with just a name is a symbol. Eg, ``#foo``.
* A variant with a name and a value is a record with a single field.
  Eg, ``{bar: x}``.

<give an example: a variant type, and pattern matching>

For example, the ``align`` operator aligns a shape with respect to the origin,
along one or more major axes. The alignment with respect to a specific axis
is specified using an alignment value, which is a variant:

* ``#centre`` -- Centre the shape on the origin.
* ``{above: d}`` -- Position the shape so that its bounding box
  is distance ``d`` above the origin.
* ``{below: d}`` -- Position the shape so that its bounding box
  is distance ``d`` below the origin.
* ``{within: k}`` -- Align the shape so that the origin is at position ``k``,
  where -1 is the bottom of the bound box, 0 is the centre of the bounding box,
  and +1 is the top of the bounding box.

-------------------------------------------------------------

A variant value is queried using pattern matching:
    match [
      #niladic -> [],
      {monadic: a} -> [a],
      {dyadic: (a,b)} -> [a,b],
    ]



`#foo` is a symbol value.
Symbol values are used when you need a C-like enumerated type.
Note, #foo is intentionally similar to a Twitter hash tag.

Symbols are used to simulate 'enumerated types'.
Symbols are used to simulate niladic variants in an algebraic type.

Symbols are used to simulate 'enumerated types' in SubCurv.
Drop-down-list picker type. In a C-like language, the options in a drop-down
list would be enums, which would map to integers in GLSL. So these should be
symbols?
    parametric {s :: dropdown[#square,#circle,#triangle] = #square} ...
In SubCurv, symbols are normally illegal, and even if we get record values,
the null value in a symbol is illegal. But, if a dropdown picker is used,
then the symbols it uses become legal SubCurv values, represented as integers.
We could also use these semantics for interpreting an is_enum[#foo,#bar]
type predicate in SubCurv.

3. Symbols as an Abstract Type
------------------------------
Sales pitch:
* Symbols are fully abstract, simple, scalar values.
  The only Symbol operations are construction, equality, conversion to string
  (which are the generic operations supported by all values).
* #foo prints as #foo, is only equal to values that print as #foo.
  There's no aliasing with other types. Simple.
* (Why not use strings?) Symbols do not compare equal to strings.
  Overloaded functions can distinguish symbol arguments from string arguments.
* Symbols are more fundamental than strings or records, which are aggregates.
* Symbols are the natural representation for nilary enum constructors (instead
  of strings or integers).
* Symbols are the natural representation for field names in records
  (instead of strings; see Structure proposal).
* Define true=#true, false=#false, null=#null. Then, in conjunction with Maps,
  all data types have literals that can be used as patterns. (But, aliasing.)
* Symbols in SubCurv:
  * #foo is compiled to an enum value with an int representation.
  * `dropdown_menu[#Value_Noise,#Fractal_Noise]` is a picker value.
  * is_enum[#foo,#bar] is a type predicate supported by SubCurv?
* Symbols might be useful in the Term proposal.
* Symbols might have a use if Curv becomes homoiconic and supports macros?

Instances of algebraic types are notated as:
    #nilary
    {binary: (a, b)}
The field name `binary` is internally represented as #binary,
so in this sense the constructor name is always a symbol.

Construction:
    #foo
    #'hello world'

A conversion from String to Symbol? make_symbol "foo" == #foo.
This is in the same category as a conversion from String to Number.
It shouldn't normally be needed, given the role of strings in Curv.

Conversion to string:
    "${#foo}" becomes "foo"    or strcat[#foo]
    "$(#foo)" becomes "#foo"   or repr[#foo]
    `strcat[#'Hello world']` becomes "Hello world".

Field names are represented by symbols (Structure proposal).
* `fields` returns a list of symbols.

Cons:
* Explaining to users why symbols are different from strings.
  It's doable, esp. if #true and #false are the boolean values.
* Is there any context where we need a variable that is either a string
  or a symbol? Or are the use cases disjoint? (Because then why not unify them.)

JSON export: #foo -> {"\u0000":"#foo"}
                  or "\u0000foo"
or record keys are strings, no Curv value maps to JSON null, and
             #foo -> {"foo":null}

Symbols are not Strings
-----------------------
A Symbol is an abstract value whose only property is its name.
The symbol `#foo` prints as `#foo`, and is only equal to itself.
You can compare a symbol for equality to any other value, use it as a map key,
or convert it to a string. Those are the only operations.

Symbol constants look like Twitter hash tags, and that's not a coincidence.
Symbols are abstract names that have semantic meaning within a program.

In Curv, the Boolean values are called `#true` and `#false` (they are symbols),
and this is a good example of what symbols are used for. They are used to
distinguish between several different named alternatives.

Statically typed languages like C, Rust, Swift and Go do not have a generic
symbol type. Instead, they have user-defined `enum` types, which serve the
same purpose. Internally, `enum` values are represented efficiently by small
integers. When Curv programs are compiled into statically typed code (eg, into
C++ or into GLSL), symbol values are compiled to small integers or enum values.

The only other languages with a symbol type are dynamically typed languages:
* Lisp, Scheme and other languages from the Lisp family.
* Ruby.
* Erlang and Elixir (where symbols are called "atoms").
Javascript has a Symbol class, but it is an unrelated concept.

When users first encounter Symbols in a language like Scheme, Elixir or Ruby,
it can be unclear how Symbols differ from Strings. In Curv, the distinction
is very clear.

Strings are meant to represent uninterpreted text that is destined to form
part of the program's output.
* Documentation/help strings (in a future language version).
* A string of text that will be rendered into an image using the future `text`
  primitive.
* A string of text that will be printed as a debug message.
* A string of text that represents the final output of a program
  (in the case where you are using Curv to convert your data to some text
  based file format for further use outside of Curv).

You are not meant to parse strings. Curv has no way of opening and reading a
text file, so there's no input to parse. Curv isn't a text processing language,
and doesn't have regular expressions or parsing facilities.

You are not meant to use strings to encode meaning within your data structures.
* You shouldn't internally represent a compound data structure using Strings,
  because now your code has to parse that string to traverse the data structure.
  That's a code smell, because parsing is complex and error prone compared to
  just traversing a real data structure.
* You shouldn't use strings to encode semantically meaningful names, eg denoting
  one of several alternatives. That's what Symbols are for. If your code
  compares two strings for equality, or uses a string as a map key,
  then you should use Symbols instead.
Break free of JSON and support a more expressive system of fundamental types.
* Maps (aka Dictionaries)
* Sets (maybe)
* Symbols (maybe)
* Some data types have associated literal patterns.
* New function equality.
* Quoted identifiers.

Symbols.
--------
* Symbols are abstract values, distinguished only by their name.
  They only support equality and conversion to and from strings.
  #foo is a symbol; it prints as #foo; it is only equal to itself.
* #'hello world' is a symbol with nonstandard name. Used with dropdown_menu
  proposal: `dropdown_menu[#'Value Noise', #'Fractal Noise']`.
  (Note, not #"hello world" as that conflicts with the proposal for adding
   Swift5 string literal syntax.)
* Define true=#true, false=#false, null=#null.
* #foo is a pattern.
* Thus, #true and #false are the literal pattern syntax for booleans.
  (An alternative is for true and false to be keywords.)
* Record fields are symbols internally.
* `is_symbol x`

Variant Values (and Variant Types)
----------------------------------
Variant types are a feature of statically typed languages.
They have many names: discriminated unions, enums, variants and algebraic data types.
The core idea is that a variant type consists of a fixed set of alternatives.
Each alternative has a name, and optionally a value.
An enum type is one in which all of the alternatives are just names.

A variant (or variant value) is an instance of a variant type.
To construct a variant, you need to specify the name of one of the alternatives,
plus a value, if that alternative carries a value.

In Curv, we don't have explicitly declared variant types. Instead, variants
can be constructed directly. A variant with just a name is a symbol.
A variant with a name and a value is a record with a single field.
    #niladic
    {monadic: a}
    {dyadic: (a,b)}

A variant value is queried using pattern matching:
    match [
      #niladic -> [],
      {monadic: a} -> [a],
      {dyadic: (a,b)} -> [a,b],
    ]

I'd like to define a picker that takes a variant type as an argument.
It displays a drop-down menu for the tag, plus additional pickers for
data associated with the current tag value.
    variant_picker [ alternative, ... ]
Each alternative is either a symbol, or {tag: {record of pickers}},
or {tag: [alternative, ...]} if we want an alternate form of nesting.
The parameter that is bound to a variant_picker has a variant value that
must be queried using `match`.

Variant values are abstract: you use pattern matching to query the value.
There is another kind of tagged value which preserves the operations
on the value (eg, Cell tagged values, or Curv terms).
