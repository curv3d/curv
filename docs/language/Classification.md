# Classification of Values

Built-in types are true abstract data types. They are marked with type codes
by the implementation in a way that can't be counterfeited from within the
language. Built-in predicate functions like `is_bool` and `is_num`
are provided to test these type codes.

There are no user defined abstract data types in TeaCAD.
In fact, there is no such thing as a type at all, there are only
predicate functions. The additional complexity is not justified
in a simple domain-specific language like this.

(As soon as you introduce user-defined types with unique unforgeable type codes,
you lose referential transparency, which is part of the complexity.)

The only tool for "user defined types" that we provide are objects.
An object is just an immutable set of name/value pairs, like a JSON object.
This is enough. The F-Rep system uses objects in a sophisticated way.

For F-Rep objects, I need a system of inherited and synthesized attributes,
inspired by AST technology.

Synthesized attributes:
* An object synthesizes an attribute (which classifies the object)
  based on its implementation, and the attribute values of the same name of
  its component objects.
* We use boolean fields. Each field needs a predicate for testing
  an object, returning the boolean field value or false if it isn't defined.
  Need an operator to test if an object defines a field.

Inherited attributes:
When you construct a shape's distance field,
you can specify requirements, like the outside should be euclidean.
* An F-Rep shape object has fields representing parameters, child shapes,
  and synthesized attributes,
  plus a member function `get_distance_field(requirements)`.
* `get_distance_field` chooses an algorithm to implement its distance field
  based on bottom-up synthesized attributes (stored in its children)
  plus top-down inherited attributes specified by its parent
  (the `requirements` argument).
* It can augment the requirements specified by its caller with additional
  requirements before passing these down to child shapes via their
  `get_distance_field()` calls.
* The system for defining attributes is decentralized.
  A specialized library might invent new attributes, which are defaulted
  in shapes not designed for use with this library.
* The `requirements` argument is an object with one field for each requirement.
  They are usually boolean and default to false if not defined.
* There might also be numeric attributes. Like an error bound on how
  euclidean the outside should be. Again with a default if missing.

So, synthesized and inherited attributes are just object fields.
We need a mechanism for fetching an attribute value,
with a default value if the field is not defined.

Some syntax:
* `? obj.id` -- true if field `id` is defined, else false
* `? obj@"id"` -- true if field `id` is defined, else false
* `obj.id ? default` -- value of obj.id if defined, else default
* `is_polyhedron(X) = X.is_polyhedron ? false;`

We also need a mechanism for extending an existing object with new
or overridden fields.
In OpenSCAD2, the expression form is `base overlay extensions`,
and the statement form is `overlay extensions;`.
(I used a word rather than a symbolic infix operator because of readability.
A symbol like `<+` is totally cryptic.)

Alt syntax: test if an object field is defined:
`defined(<selection>.<identifier>)`.

It looks like almost the entire F-Rep geometry system could be defined
using TeaCAD code. I need two primitive functions, `mk2dshape`
and `mk3dshape`, which take an object as an argument, check that
get_distance_field is defined and has the right type, abort
if the distance field uses features not portable to GLSL, return
the object branded as a shape. I also need `is_shape`.
