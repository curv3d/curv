# Parameterized Shapes
This describes an extension of the shape library in which each shape retains
its model parameters. It can be exported as a CSG tree, customized, etc.
You can traverse the CSG tree and extract model "metadata".

## Requirements
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
  * Might need to contain a URL?
* Can be printed as a Curv expression, with model parameters.
  If this is the default presentation (ie as a CSG tree in Curv syntax),
  then use `merge <shape>` to convert to a record and see the shape attributes.

## Design
Shape Constructors:
* A shape constructor is a value, which has attributes:
  * `name`, a string, which is the name that it is bound to
    in some scope (usually a top level module scope).
  * `arity`, a natural number.
  * If arity == 0, then a shape constructor is a shape. The above attributes
    are shape attributes.
  * If arity > 0, then a shape constructor is a function (curried if arity>1),
    and it has extra attributes:
    * `param_names`, a flat list of parameter names.
    * `make`, a constructor function that maps a flat parameter record
      onto a shape. Used for importing JCSG.
    * `print`, a function that maps a flat parameter record onto a (possibly
      curried) Curv funcall expression that reconstructs the shape.
* A shape constructor is printed as its name, and compares equal only to itself.
* A fully constructed shape (constructed from a shape constructor):
  * prints as a Curv expression that evaluates to that shape. Id est,
    it prints as a CSG tree in Curv syntax.
  * compares equal only to other shapes that print the same way. So we compare
    the constructor and the parameters for equality.
* For shape constructors with arity > 1, we can also have partially constructed
  shapes, which are functions. These are a kind of shape constructor, kind of a
  cross between a normal shape constructor and a shape.

Shape Arity:
* `everything` is a niladic shape, and is a niladic shape constructor.
* `square` is a monadic shape constructor, and `square(10)` is a monadic shape.
* `translate` is a dyadic shape constructor.
  `translate[10,10]` is a partially constructed shape.
  `translate[10,20]square(10)` is a dyadic shape.

Shape Attributes:
* `arity`, a natural number.
* If arity == 0,
  * `name`, a string.
* If arity > 0,
  * `constructor`, a shape constructor value
  * `param`, a parameter record

Defining a Shape Constructor:
* `defshape` takes care of all of the above details.
* `defshape name (parameters&default-values) = body`.
* eg, `defshape everything = make_shape { dist=.., bbox=.. };`
* eg, `defshape square(sz) = make_shape { ... };`
* eg, `defshape translate(v)shape = make_shape { ... };`
* eg, `defshape box(sz) = cube(sz);` // exported as `box`, not as `cube`

Partially Constructed Shape Representation:
* TBD

Shape Fields:
What are the fields of a shape value? How do we access parameters,
vs how do we access attributes?
 1. The fields are the attributes. `shape.param` are the parameters.
    I am okay with this. It's the simplest design.
 2. The fields are the parameters. `attr(shape)` are the attributes,
    and `attr(shape).param` is an alternate path to accessing the parameters.
    This alternative is needed if Module is a subtype of Shape, in support
    of the OpenSCAD2 style of programming. Also, you'll print `shape` to see
    the parameters and `attr(shape)` to see the attributes.

Customization:
 * The above design already supports `shape.constructor.make(paramrec)`
   and `shape.constructor(args)`. What does `shape(args)` mean?
   What are the requirements for this syntax, if any?
 * For a prototype-based shape library, we don't have a `cube` function,
   we have a `cube` prototype that can be customized. This goes for other
   fundamental shapes, but it mostly doesn't apply to transformations
   (operations that map shapes to shapes), like `translate`: the shape operator
   has to make sense if no arguments are specified. So no curried shape
   operators (again, like `translate`).
 * `defshape` limits support for customization to arity 1 shapes where all
   of the parameters are optional (have default values).
   In these cases, `shape(args)` means `shape.constructor(args)`.

------------------------------------------------------------------------------
## Old Text (Before 22 Jan 2017)
When we query shape parameters, how fancy is the interface?
Eg, for `u=union[a,b]`, how do we access the children of `u`?
* as `u.children`
* as `len u`, `u'i` -- this means shapes can have elements (like a module)
* If parameters and attributes share the same namespace and are both accessed
  using dot notation, how are the names distinguished?
  * parameters have a prefix like `$` or `param_`.
    This implies `u.$children` or `u.param_children`.
  * attributes have a prefix like `s_`, parameters aren't decorated.
    So, `u.children` and `u.shape_bbox`.
  * instead of a name prefix (`p_foo`) use a subrecord (`p.foo`).
    `u.param.children`.
    * The `param` attribute eliminates conflict between the attribute
      and parameter namespaces.
    * Maybe `param` is the single argument passed to the constructor,
      possibly a record, possibly a list. Eg, `u.param` is a list.

Design alternative: shapes with `param` attribute.
* `type` attribute is a string: name of constructor function, which is used
  for printing the shape as JCSG or as a Curv expression.
* If the shape is a "singleton", it has no `param` attribute, and type is name
  of a variable containing the shape. Eg, `everything`.
* What about curried operators, like `translate`?
  We'd need two param values, the offset vector and the target shape.
  Maybe `param` is a list of values. It has length 0 for `everything` (niladic),
  length 1 for `square` (monadic), length 2 for `translate` (dyadic).
* customization: 
  * A monadic shape can be called as a function, with a single argument, which
    constructs a new shape of the same kind. With a record argument, you can
    override only some of the parameters. Internally, this is implemented by
    referencing shape attributes such as `param`.
  * we look up the type (in what namespace?) to get the constructor function,
    then we apply that function to the customization argument.
  * the namespace is the namespace where make_shape is invoked?
  * there is a separate ctor attribute, which is a function. Eg,
    ```
    square(sz) = make_shape {
      type = "square";
      ctor = square;
      param = sz;
      geom(p,t) = ...;
      bbox = ...;
    };
    ```
  * `make_shape` takes an identifier argument, the ctor name in the current
    environment. Eg,
    ```
    square(sz) = make_shape (square) {
      param = sz;
      geom(p,t) = ...;
      bbox = ...;
    };
    ```
  * special definitional form which automates this further:
    ```
    shape square(sz) = {
      geom(p,t) = ...;
      bbox = ...;
    };
    ```
* What about prototype-oriented programming? `square` is a standard shape
  that can be customized.
  ```
  square = make_shape {
    type = "square";
    ctor(sz) = make_shape {
      ... code duplication ...
    };
    param = [10,10];
    geom(p,t) = ...;
    bbox = ...;
  };
  ```
  or
  ```
  shape square(sz=[10,10]) = {
    geom(p,t) = ...;
    bbox = ...;
  };
  ```
* With this design, a module is not a shape (module is not a subtype of shape).
  Modules are used as libraries, so they must export their definitions as
  fields. A module can be explicitly converted to a shape.

## Modules
In OpenSCAD2, a script denotes a module, which denotes a parameterized shape.
Submodule syntax is a convenient way to construct composite shapes and pass
them as arguments -- it also supports prototype based programming, which I
proposed as easier for beginners than defining functions.

So, a module behaves like a shape.
* The module directly defines the parameters.
* A union of the module elements defines the attributes.
* The type is 'module'.
* The shape is serialized as a module, with the parameters and the shape
  elements. (The parameters are model parameters, aka 'metadata'
  that's missing from groups in OpenSCAD CSG output.)
* The data that appears in the serialization can also be queried.
* Based on this, the serialization of a shape module doesn't fully
  reconstruct the original module. It lacks the code that converts the
  parameters into the shape elements. (Probably okay; see "define parameterized
  module" for how to fix this.)

In what contexts is a module a shape? Two alternatives:
* There is an explicit operation that converts a module into a shape.
  Call this `emshapen module` (for now).
* There is an implicit conversion from module to shape.
  Shape attribute queries are distinguished from module field references.
  * shattr(shape) -> shape attribute record. I guess that shape fields
    are just the parameters, maybe as specified by the `param` attribute.
  * shattr(module) -> module union attribute record
  * I can/may want to wrap attributes in access functions, at least according
    to an earlier design. Eg, `sh_bbox sh = sh_attr(sh).bbox`.

Can we define a parameterized module:
* that defines a composite shape using high level operations (not using the
  shape protocol), and which can be serialized into a Curv expression that
  reproduces the original value?
* where the shape has a user-defined type different than "module"?
  This is needed for exporting a model containing user-defined high level
  geometric operators.
* This type is presumably the name of a binding (function or module)
  which maps the shape parameters onto the original shape.
  * Do we support curried constructors here? Maybe the constructor is a
    named function. Or maybe it's a named module--can we extend module
    customization to support currying?
* This is a more advanced feature than prototype-oriented programming
  in OpenSCAD2, so it can be more complicated to use.
  We don't need module customization to support currying.
  The constructor can be a normal function.
* So I'm back to providing a special definitional form for "shape classes".
  `defshape name (parameters&default-values) = body`.

## 20 Jan 2017
Where I am so far:
* A shape has an arity, determined by its constructor. `everything` is niladic,
  `square(10)` is monadic, `translate[2,3]shape` is dyadic.
  * A dyadic shape serializes as a Curv curried funcall.
* Regardless of arity, each shape class has a set of disjoint parameter names.
  When a shape is serialized as JSON (dyadic or otherwise), the parameters
  become keys in a JSON object, along with a "$type" field.
* Shape attributes include type name, constructor function, parameters, arity,
  which are used internally for serialization and customization.
* A high level definitional form for shape constructors builds these attributes
  automatically. `defshape name (parameters&default-values) = body`.
  This works for niladic, monadic, dyadic, etc.
* What are the fields of a shape value? Two choices:
  1. the attributes (use `shape.param` to access the parameters)
  2. the parameters (use `sh_attr(shape)` to access the attributes).
     Advantage: module is a subtype of shape.
* How does customization work for dyadic shapes?
  * You can selectively override any parameter by name. Parameters not
    specified keep their current value. Argument order doesn't matter.
  * Thus, for dyadic shapes, this is not a curried function, requiring
    ordered parameters. Consistency with the constructor isn't required.
* How is a script module converted to a shape? Do we support OpenSCAD2 style
  geometric objects? Choices for converting a module to a shape:
  * Explicit. emshapen(module)
  * Implicit. module is a subtype of shape. Use `sh_attr(shape)` to get
    the attributes of a shape.
  * Limitations: a shape specified this way has "module" as its "$type" and
    can't be dyadic. Use `defshape` if you want control over the constructor
    name and arity.

## 21 Jan 2017

## Older Notes, Not Yet Updated and Merged into Current Text
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
  Right now it's `make_shape <record>` and `shape.attribute`/`shape.param`.

Here's a design:
* Shapes are branded modules. A shape has fields and elements
  and can be customized.
* Parameter fields are marked by a name prefix, eg `$radius` or `param_radius`,
  and not by using a `param` keyword.
* `emshapen <module>` constructs a new shape containing the original module
  plus the result of unioning its elements. The fields of this shape consist
  of the module parameters plus the union attributes. The elements of this
  shape are the module elements.
* Maybe there is a special curv file extension that means: interpret this curv
  script as a shape. And another extension for libraries.
