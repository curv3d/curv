# Objects

An object consists of a map from names to values (these are the fields),
plus an ordered sequence of values (these are the elements).

A simple object, with 2 fields (x and y) and 3 elements (1,2,3):
```
obj = {x=true, y="blue", 1, 2, 3};
```

Objects are a generalization of lists. An object is like a list
with the addition of metadata (name/value pairs).
When list operations are applied to an object, they act on the elements.
```
len(obj) => 3
obj@0 => 1
elements(obj) => [1,2,3]
[for (elem=obj) elem] => [1,2,3]
```

Fields can be referenced using dot notation.
Here are the field operations:
```
obj.x => true
obj.("y") => "blue"
defined(obj.x) => true
defined(obj.("z")) => false
fields(obj) => ["x", "y"]
[for (key=fields(obj)) obj.(key)] => [true,"blue"]
```

There are two kinds of object, simple and scoped.
They have different constructor syntaxes and internal representations,
with different performance characteristics, but the two kinds are
interchangeable, and there is only one object type.

## Simple Objects

A simple object constructor is a set of zero or more field definitions
and element expressions separated by commas.
The order of the definitions doesn't matter.
```
{ a = 1, b = 2 }
```
There is no scope introduced inside the constructor.

Simple objects are used to represent sets of model parameters,
and for the requirements object in the shape3d protocol.

The representation is probably a binary tree with refcounted nodes,
plus a list.

## Scoped Objects

Scoped objects extend simple objects with dependencies and parameter fields.
You can customize a scoped object using function call syntax, specifying
new parameter values as arguments, and a new object will be constructed
(with all dependent fields and elements updated with the new parameter values).
The use cases are parameterized shapes and libraries.

A scoped object constructor introduces a lexical scope, so that
field definitions can refer to each other, which creates dependencies.
The scope of each definition is the entire constructor, and the order
of definitions doesn't matter.
There are two kinds:
* a script file, which is a sequence of statements.
* `{`*script*`}`, which contains one or more statements.

For example, `{x=1; y=x+1;}` is a scoped object where `y` depends on `x`.

If a definition is prefixed with the `param` keyword, then it is a
parameter field.

Statements are terminated by semicolons.
A statement is:
* a definition, x=0; or f(x)=x+1; with optional `param` prefix.
* `use` statements (library support)
* `include` statements (object concatenation; inheritance semantics)
* side effects: assert and echo
* expressions and generators, like in list constructors

`include x;` requires `x` to be a list or object.
* It interpolates the elements of `x` into the base object's element list
  at the point where the `include` statement occurs.
* If `x` is an object, then the effect is to concatenate the fields onto
  the base object, in the manner of `concat[base,x]`.
  If an object literal contains include statements, then the field values
  of the resulting object are determined by first processing all of the
  definitions, then processing the first include statement and concatenating
  those fields (they will override definitions from the first phase),
  then processing the second include, and so on.
* In the initial TeaCAD implementation, we may restrict `x` to be only
  a list, or to be only a simple object (no dependencies or parameters).

The representation is a slot vector and a dictionary.
* The dictionary is shared by all clones of the object, which speeds up
  and reduces the memory cost of customization.
* The dictionary contains some representation of the parse or meaning tree
  for each definition, used for implementing inheritance (include and overlay).

The order of definitions doesn't matter (as far as dependencies go);
the scope of each definition is the entire script.
There are *no* restrictions on recursive definitions, like Haskell.
That means `x=x;` is legal and `x` is an infinite loop.

The order of actions is relevant: list items are ordered, side
effects are ordered. The side effects actions are evaluated when the
object constructor is evaluated. The other things are

A script file denotes a scoped object, but in this special case,
top level definitions are lazy, stored as parse trees until first
reference, when they are JIT compiled.

Idiom: a function constructs an object, the function parameters have
the same names as corresponding object parameters. How is this possible?
```
(a) f(a) let(outer_a=a) {a=outer_a;} // a clumsy idiom, proof of concept
(b) f(a) {a=outer.a;} // outer is a keyword
(c) f(a) {a=a} // simple objects don't introduce a scope
```

Parameterized objects. One or more definitions can be prefixed with `param`,
which makes them into parameters. An object can be cloned or customized
using function call notation, replacing parameter values with new values.
* This is relevant to the GUI customizer: only parameters can be tweaked
* Parameters are compiled differently.
  They are always allocated to slots, even if they have a static value.
  This means the customizer gui can run without recompiling the script.
* The order of parameter definitions matters for passing them as positional
  arguments during customization.

## Object Concatenation

An object is a set of name/value pairs, plus a sequence of values.
Objects are a generalization of lists. This is reflected in the `concat`
operator, which concatenates objects, lists, or a mixture of both.

semantics of `concat[a,b]`:
* It's associative, but not commutative.
* `[]` is the identity element.
* `concat[obj,list]`: `list` is appended to the end of `obj`s element sequence.
* `concat[list,obj]`: `list` is prepended to the beginning
  of `obj`s element sequence.

semantics of object concatenation. `concat[base,ext]`:
* The resulting object has the union of the fields in base and ext,
  with the field values of ext taking precedence and overriding the base.
* The elements of the resulting object
  is concat[elements(base), elements(ext)].
* `{}` is an identity element for object concatenation.

In the initial TeaCAD implementation, object concatenation will
always produce a simple object. Possibly we abort if argument object has
dependencies and/or parameters? (This would be independent of the syntax
of the original constructor.) This implies a predicate to test if an object
has dependencies or parameters. It also violates the principle that a
scoped object is interchangeable with a simple object (dependencies and
parameters are ignored in contexts where they are irrelevant).
Maybe there is a different, more expensive operation for composing
scoped objects, which reduces to `concat` in the simple case.
Although that introduces a picky, fine grained distinction that
for simplicity should be avoided.

Later, we may try a more sophisticated design that preserves
the dependencies and parameters of scoped objects:
* If the base has parameter fields, then the result has the same parameter
  fields in the same order. The result can be customized using the same
  argument lists that work for the base.
* If a list and a scoped object are concatenated, the resulting object
  is scoped and preserves the input object's dependencies and parameters.
* If base and ext are simple, the result is simple.
  This operation is cheap: maybe a few tree nodes are copied, and linear
  update requires no copies.
  Concatenation with scoped objects is more expensive.
* If base is simple and ext is scoped, the result is scoped.
  The result has the same dependencies and parameter fields as ext.
  (Necessary because `{}` is an identity element.)
* If the base is scoped, then the result is scoped.
  Dependencies within ext are copied into the result.
  The parameter status of fields within ext are ignored if those fields
  override existing fields within base. However, ext can add new parameter
  fields to the end of base's parameter list.

What happens when you concatenate to or from a shape object?
* Shape objects are internally branded by the shape3d operator,
  and are members of the `is_shape` predicate.
* Let's consider how user-defined object branding works.
  You define a predicate, eg:
        ```
        is_polytope(x) = if (defined x.polytope) x.polytope else false;
        ```
  then you define `polytope=true` to brand your object.
  This style of branding can be inherited from either the `base` or `ext`
  arguments of object concatenation, as long as its not overridden to false
  by `ext`.
* By analogy, `concat[base,ext]` returns a shape if either `base` or `ext`
  is a shape. `shape3d` is run on the result to verify that it's a valid
  shape, and that verification may fail.
* This is consistent with `{}` and `[]` being identity elements.

For example, `concat[cube(10),{meta=42}]` shows how to add metadata
to a shape. Of course, you need to make sure not to interfere with
a field needed for the shape's correct operation.
An alternative is `concat[{meta=10},cube(10)]` which guarantees not to
override an existing field, only to add a new field if it wasn't present.

## Customization:
* Invoke an object like a function, this creates a clone of the original
  with each argument replacing an existing field that is declared as `param`.
  This is only useful with heavy objects, and is designed to be cheap.
* Simple object literals produce objects with no parameters.
  Any object can be invoked as obj(), which just returns obj.

## Libraries
* `use obj;`

optional: `only(id1,id2,...)obj` and `except(id1,id2,...)obj`

## Inheritance
This is the most complex and heavy weight part of the OpenSCAD2 object design.
It's the lowest priority.
* `include obj;`

I don't have a use for OpenSCAD mixins, not for my own stuff.

## Old Design Process
Modifying an object: fast runtime operations.
These operations treat an object as a map of independent key-value pairs.
You can update one of these pairs without affecting the others.
Source code dependencies between definiens don't matter here.
You can also add new key-value pairs.
* `obj(arg1,arg2,...)` -- "customization". Fields defined as 'param'
  can be overridden. It's fast: we allocate a new slot array but share
  the original dictionary that maps names to slot ids. The new values
  are stored in slots.
* `obj1 <+ obj2` or something. Override arbitrary fields, add new fields.
* The OOP and inheritance-oriented object representation might not make
  sense. Instead, maybe use a std::map, copy on write, mutate if refcount==1.
* There's a temptation to create a different map type, but I don't want to
  do that. If I have two map representations, how do I choose which to use?
  Maybe it's a syntax distinction without a type distinction,
  like cons() vs concat() for lists. {a=1,b=2} creates the std::map
  representation, where linear overlays are cheap, vs the `;` syntax.
  But they are the same type, semantically interchangeable.
* Haskell Data.Map is a binary tree, similar to std::map. On an insertion,
  only a few nodes need to be copied. Clojure uses hash array-mapped trie.
* With the {,} syntax, there are no dependencies between fields,
  which are important to the 'inheritance' operators.
  Also no params, no actions.
  Eg, {x=1,a=x} means:
  * No new scope is introduced, the x in a=x references an external x.
    That would also mean no recursive definitions, no thunks.
  * Or, we use recursive scoping, but the dependencies between definiens
    are thrown away when the map is constructed. But, that creates an
    implementation difficulty for recursive definitions, due to rthunks
    needing a slot vector.
* If I have a 'simple' and a 'scoped' object notation,
  with different internal representions, then
  what happens if they are combined? Compile time shit happens when heavy
  objects are combined, so that happens here too.

Extension operations:
* `obj1 overlay obj2` or `{include obj1; overlay obj2;}`
* As a change to OpenSCAD2, maybe `{include obj; x=a;}` overlays the
  new definition of `x`. This is closer to original OpenSCAD semantics,
  and preserves the order independence of OpenSCAD2?
  The original design document claims this is confusing, and suggests
  `include obj1 overlay {x=a};` so it's clear that x=a is an override,
  and so it's clear which object it is overriding.
* `include`, `use` and `overlay`.
* I have plans to compile objects into optimized code, with static definitions
  treated as compile time constants that can be constant folded, etc.
  Use `param` to allow run-time customization. This means that `include`,
  `use` and `overlay` may involve recompiling the amended script.
* Is there a fast overlay operator that doesn't involve recompiling?
  I'd want that for shape3d. To be clear, this means different semantics.
  I'm treating the object as a map containing an independent set of values,
  and overriding values in the map doesn't re-evaluate dependent expressions.

## overlay
How do param definitions and overlays interact?
* for shape3d::dist::req, I don't care, I won't be using param definitions
  together with override.
* If an object has parameters, then an API is established for customizing
  the object. An object is no longer a simple map. Overlay is more like
  inheritance and override in an OOP language.
  * Probably, you don't want to break the API, just extend it in an
    upward compatible way (as in interface inheritance).
    `{include obj1; overlay obj2;}` is interface inheritance and override.
  * Possibly, you intend to do implementation inheritance and establish
    a new API. But then, you should `use obj1;` and then define all of
    fields in the new API with reference to obj1. You get to define
    from scratch which fields are parameters.
* case analysis:
  * non-param overlays param, what happens?
    * Is the non-param/param property overridden?
      * In the common case, I think you are specifying a new value for
        an existing parameter without changing the inherited API.
      * If this makes sense, then wouldn't we also provide a way to
        deparameterize an inherited parameter without specifying a new value?
      * Yes, because this lets you make changes to that property in a general
        way.
      * Note that customization, obj(x=a), does not change the property.
    * Does the relative location of the field in the param order change?
      * ACCEPT: No, because then you can't in general override single parameter
        values without also changing their order. The order might be
        an important aspect of the API, and you don't want to break the
        API just to override a field. (To preserve order you
        could override all existing parameters, which is a clumsy requirement
        to avoid breaking an API. Also, you might not know all of the field
        values, as is the case in a shape3d requirements object.)
      * REJECT: Yes, because this lets you change parameter order. But changing
        the parameter order is a big deal, it should require a heavier weight
        syntax. In this case, it would make sense to restate all of the
        parameters when establishing a new order.
  * param overlays non-param, what happens? (ditto.)
  * new param field: how is it ordered relative to inherited param fields?
    * New param fields always come after base param fields,
      to avoid breaking the API.
    * REJECT: With the action version of `overlay`, the new fields are located
      based on where `overlay` is located within the object literal
      relative to the base field definitions.

obj(x=a) is a bit like concat[obj,{x=a}].
