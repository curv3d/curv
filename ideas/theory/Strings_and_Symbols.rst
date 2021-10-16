Strings and Symbols
===================
Curv has two 'character string' data types: strings (which are
lists of characters), and symbols (which are abstract names with
no internal structure).

Why do we need both?
It's due to the fact that strings can play two different roles in programs:
that of a list of characters, manipulated like an array, and that of a
name or identifier. The API requirements for these two roles are different.
Trying to force a single data type to play both roles creates a messy
compromise that is both more complicated and less versatile.

A String is a List of Characters
--------------------------------
Many well known dynamically typed languages have a string type that
is distinct from their list type, and do not provide a character type.
Examples include: Python, Javascript, Ruby.

In these languages, the string API is irritatingly close to the list API
without matching it exactly. For example, if you write a function to reverse
a list, then, depending on how you wrote it, it may or may not work to also
reverse a string. You have to keep in mind the differences between the list
and string API in order to succeed in writing a reverse function that works
on both. This is added complexity, and it is a burden on the programmer.

The solution is to introduce a character type, and to redefine a string
as a list of characters. This is the approach taken by Curv, in which
a string literal like ``"abc"`` is simply shorthand for a list literal
like ``[char 97, char 98, char 99]``. The primitive types here are list
and character: there is no primitive string type.

"Hybrid Strings" are Useful for Visual Programming
--------------------------------------------------
Because a string is just a list of characters, and lists can contain any
kind of data, it is possible in Curv to create "hybrid strings", which are
lists that contain a mixture of character and non-character data.
Since this is impossible in Python, Javascript, Ruby, and many other
dynamically typed languages, you may ask why Curv needs this feature?

The answer is that I'm laying the groundwork to evolve Curv into a more
visual programming language with a GUI based live programming environment.
And for that, I need to be able to embed "live values" inside of character
strings.
 * Error messages and debug log messages created by the ``print`` statement
   often contain embedded values. Right now, these messages are just character
   strings, and the embedded values are 'dead': they are serialized text,
   not inspectable values. That will change.
 * Curv will support 'doc strings' for functions and modules, a feature
   found in Lisp, Python and other full-featured dynamic languages.
   But it's too limiting to restrict doc strings to character data. This is
   documentation, and we want embedded images, and embedded interactive
   live values.

A Symbol is an Abstract Named Value
-----------------------------------
A *variant value* is a value that represents one of several disjoint
alternatives. Each of these alternatives has a name.
Sometimes an alternative is just a name, without any additional data.
And sometimes an alternative is a name plus additional data.

In statically typed languages, you model a variant value by defining
a variant type, also called a sum type or an enum type.
For example, in Haskell, you define a sum type like this::

    data Edge = Sharp | Rounded Double

and then you can write ``Sharp`` or ``Rounded 1.5`` to describe an edge effect.

Dynamically typed languages have *informal* variant values.
You don't have to define a type: all the variant values you will ever need
are predefined.

In Curv, you construct variant values like this:

  #sharp
  rounded: 1.5

``#sharp`` is a symbol and ``rounded: 1.5`` is an association.

The requirements for variant values in a dynamically typed language are:

* They are abstract. A symbol is an abstract named value; it is equal to
  another symbol with the same name. It is unequal to any other value.
  Likewise, an association is equal to another association with the same
  label and the same payload value, but is unequal to any other value.
  Variant value types are disjoint from all other data types.
* They support pattern matching.

In Javascript, there are a handful of built in abstract named values,
which have the same properties as symbols. These include ``null``,
``true`` and ``false``. For uses of symbols, it is conventional
practice to use strings.
