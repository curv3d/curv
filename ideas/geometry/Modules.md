# Modules are Shapes & Prototype-Oriented Programming

This is about convenient notation and making things easier for beginners,
not adding expressive power.

## Scripts denote Shapes
In the OpenSCAD user experience, you type in a script (with parameter settings
and shape expressions), press Render, and you see a shape.
So here, a script evaluates to a shape.

At minimum, we need a way to reference external script files as shapes.
If module values support the List protocol, then a simple solution is:
   `union file(filename)`

Can we use scripts as a notation to construct sub-shapes?
 1. Implement the OpenSCAD2 design: `{script}` is a module value; it exports
    its definitions as fields, so it behaves like a library; it also behaves
    as a shape (as the implicit union of its shape elements). See below.
 2. Different syntax for libraries and shapes constructed from a script.
    Eg, an explicit operation to convert a module value to a shape.
    Equivalent to `union`, unless I don't want modules to use the List
    protocol to export their elements.

Use cases:
 * Use a script to construct a subshape as an argument to a shape operation.
   Script definitions are effectively used as local variables.
   Multiple shape elements are implicitly unioned.
   Because: notational convenience?
   * I'm ambivalent about using module bindings as an alternative notation
     for local variables in this one use case. Maybe I'd prefer a consistent,
     orthogonal and convenient notation for local variables, which works
     equally well for lists. Especially if "modules are not lists".
   * `(x=1; y=2; [x,y])`
   * `(x=1; y=2; union[s(x), c(y)])`
   * `(x=1; y=2; {s(x), c(y)})`
   * `{x=1; y=2; s(x); c(y);}`
 * Prototype-oriented programming, which is the use case highlighted in
   the OpenSCAD2 proposal. In this case, a module is bound to a name,
   which denotes a shape, but then you can create customized versions using
   the name. Can be used in conjunction with `defshape`.

What is a script? My "sequential assignment" sublanguage has while loops.
Can you generate a series of shapes from inside a while loop?
What's the syntax? (Note, I found that allowing the same phrase to be both
a definition and a generator created semantic and implementation problems,
so that's not happening for now.)

## Modules are Shapes
Module values are shapes. Module is a subtype of Shape.

* The OpenSCAD2 syntax for a submodule is `{<script>}`.
  However, `module {<script>}` would also work in Curv.

* Modules export their definitions as fields. Therefore, module parameters
  are fields. Therefore, shape parameters are fields. To avoid namespace
  collisions, `attr(shape)` is used to access shape attributes (the low level
  shape protocol). This is okay: the high level interface (parameters) is more
  accessible than the low level interface (attributes).

* A module does not preserve its identity in a JCSG export unless wrapped
  by a shape constructor using defshape. Otherwise, only the module's shape
  elements appear, as a union if there are more than one, or as `nothing` if
  there are zero elements.

* Thus, in this sense, a module M has a single shape S.
  `attr(M)` accesses the attributes of S, and M is operationally the same
  shape as S. Except that M and S have different parameters,
  and different behaviour on customization.

## Modules are not Lists
In OpenSCAD, a `{...}` statement list containing multiple shape statements
is either interpreted as a list of individual shape arguments to a module,
or they are implicitly unioned into a single shape, depending on context.

For backwards compatibility, OpenSCAD2 works the same way. An object is a
shape, and it is also a list. Primitive shapes are singleton lists.

If Curv worked this way, then modules and shapes are both lists.
In OpenSCAD, the confusion between shapes and lists actually causes problems,
eg requiring kludges like `intersection_for`. That's fixed in OpenSCAD2, but
I don't see the need for this in Curv. It's cleaner if lists and shapes are
disjoint. Makes it easier to walk a CSG tree.

This suggests that modules are not lists. Maybe use `elements(module)`
to access the elements. The syntax isn't critical.

[Modules as lists are kind of like the ability to add metadata to a list.
Javascript objects, Lua tables and Clojure have that, but most languages don't,
and I don't see the need for Curv. It also kind of complicates generic
operations on lists. Eg, should I have generic "sort" and "reverse" operators
that preserve "list metadata" in the result?]

## Convenient Notation?
In OpenSCAD, the `{...}` notation serves to group a list of shapes passed
as an argument to a module, and you can also define local variables.
If a group is used in a single-shape context then it is implicitly unioned.

Currently in Curv, these notations are written as
```
   [ list of shapes ]
   union [ list of shapes ]
   let (locals) union [ list of shapes ]
```
There's no implicit union except at the top level.

In OpenSCAD2, the `{...}` notation is a "geometric object". It satisfies all
the same needs, and in addition, local variable bindings become fields
and parameters. With customization, this is the basis of prototype oriented
programming.

I *think* that OpenSCAD2 shapes always behave like lists.
So len(cube)==1 and cube'0==cube. For compatibility with how `children`
works in OpenSCAD? Is this useful in Curv, where I don't care about OpenSCAD
compatibility? Does it make sense? (Implicit union of a list is one thing,
implicit listification of a non-list is another thing.)

What are the issues here?
* **Implicit union** of a listy thing of shapes. Same value is listy and shapey.
  Eliminates need to explicitly write `union`.
  Consistent with the programming model for top-level modules.
* **Prototype oriented programming**.
  Specify a parameterized shape by writing some parameter definitions
  then writing some shape expressions that reference the parameters.
  Consistent with the programming style for top-level modules.
* **Customization**: `cube` and `cube(10)`, and ditto for shapes defined
  by thingiverse scripts.
* **Local variables** within a shape expression, using convenient syntax.
  `let` is a bit clumsy.
  Consistent with the programming model for top-level modules.

What do I want to do in Curv?

* I could adopt the OpenSCAD2 "object" notation, constructing a module value.
  Module is a subtype of List and is also a subtype of Shape.
  There's an implicit union when a Module is silently coerced to a Shape.
  Shape fields are now parameters; `attr(shape)` is used to access attributes.

* OpenSCAD2 lets you customize any field of an object.
  More recently I've considered marking parameter fields with a `param`
  keyword.
  * This interacts better with a component-oriented GUI, customizer, visual
    programming environment, where each shape/component has a marked set of
    parameters that are presented by the GUI for modification.
  * It might be relevant to compilation and performance? Non-parameters that
    are constant expressions can be treated as constants and folded.
    The GL compiler doesn't care, so this might be irrelevant.

## Ways to define parameterized shapes
Function vs prototype, with/without defshape.

OpenSCAD2 object:
```
lollipop = {
  radius   = 10; // candy
  diameter = 3;  // stick
  height   = 50; // stick

  translate([0,0,height]) sphere(r=radius);
  cylinder(d=diameter,h=height);
};
lollipop;
lollipop(height=60);
```
Curv module with marked parameters:
```
lollipop = {
  param radius   = 10; // candy
  param diameter = 3;  // stick
  param height   = 50; // stick

  translate([0,0,height]) sphere(r=radius);
  cylinder(d=diameter,h=height);
};
lollipop;
lollipop(height=60);
```
Using defshape, so that lollipop retains its identity in the CSG export.
Function version:
```
defshape lollipop{radius=10, diameter=3, height=50} =
{
  translate([0,0,height]) sphere(r=radius);
  cylinder(d=diameter,h=height);
};
lollipop{};
lollipop{height=60};
```
Prototype version:
```
defshape lollipop = {
  param radius   = 10; // candy
  param diameter = 3;  // stick
  param height   = 50; // stick

  translate([0,0,height]) sphere(r=radius);
  cylinder(d=diameter,h=height);
};
lollipop;
lollipop(height=60);
```
