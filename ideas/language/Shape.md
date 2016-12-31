# Shapes

## Parameterized Shapes
This describes an extension of the shape library in which each shape retains
its model parameters, and can be customized. It's a more complicated design,
and we don't need it any time soon.

A shape:
* Has a set of named parameters.
  * Can query using dot notation.
  * Can change using customization.
* Has a set of attributes, including `dist`.
  * Can query: dot notation or other?
* Can be serialized as JCSG (JSON format).
  Output contains type and parameters.
* Has a 'type', a string needed for JCSG.
  * This string is mapped back onto a shape constructor when we read JCSG.
  * Might need to contain a URL.
* Can be printed as a Curv expression, with model parameters?

A module behaves like a shape (in what contexts?).
* There is an explicit operation that converts a module into a shape.
  Call this `emshapen module` (for now).
  * The module directly defines the parameters.
  * A union of the module elements defines the attributes.
  * The type is 'module'.
  * The shape is serialized as a module, with the parameters and the shape
    elements. (The parameters are model parameters, aka 'metadata'
    that's missing from groups in OpenSCAD CSG output.)
  * The data that appears in the serialization can also be queried.
    Using the same syntax used to query that data from the original module?
* Is there also an implicit conversion from module to shape?
  Depends if shape attribute queries can be
  distinguished from module field references.

So, a module shape has parameter elements as well as parameter fields?

But emshapen should be implemented in Curv. Simplest code is:
```
emshapen module = union module;
```
which doesn't preserve model parameters.

Shape attribute queries.
* Using dot notation?
  If parameters and attributes belong to the same namespace, then there is a
  potential conflict between user specified parameter names in a module,
  and the attributes chosen by implicit union.
  * Maybe a naming convention is used? Maybe a sigil, like @, #, $?
  * Maybe the `param` keyword can prevent conflicts by enforcing the convention?
  * Maybe parameter names use a sigil. Eliminate the need for `param`.
    Make it easier to assign a parameter field from a function parameter of
    the same name.
* Using function notation?
  Okay, but if I introduce new shape attributes in the library, what is the
  underlying code for defining and referencing these attributes?
  Right now it's `shape2d <record>` and `shape.attribute`/`shape.param`.

Here's a design:
* Shapes are branded modules. A shape has fields and elements
  and can be customized.
* Parameter fields are marked by a sigil in their name, eg `$radius`,
  and not by using a `param` keyword.
* `emshapen <module>` constructs a new shape containing the original module
  plus the result of unioning its elements. The fields of this shape consist
  of the module parameters plus the union attributes. The elements of this
  shape are the module elements.
* Maybe there is a special curv file extension that means: interpret this curv
  script as a shape. And another extension for libraries.
