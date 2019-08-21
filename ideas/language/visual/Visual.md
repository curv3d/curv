Visual Curv
===========
Visual Curv is a dialect of Curv that supports visual programming.

Visual Curv is based on Projectional Editing of source code.
<https://martinfowler.com/bliki/ProjectionalEditing.html>
* Curv programs are expressions. The source code for an expression is
  represented internally as an Abstract Syntax Tree, and then edited using
  a GUI based "projection" of the AST. This GUI is called an Editor.
* An expression is evaluated to construct a value. In general, each Editor
  will need a corresponding Viewer to display the results of running the program
  during live coding. In some cases, the Editor and Viewer might be combined
  into a single UI.

The Visual Curv GUI supports multiple Editors.
* Although some editors are universal (they can edit any expression),
  most editors are domain specific and can only edit some expressions.
* Some editors are *code editors*, which means they can edit expressions
  containing non-constant free variables. All of the universal editors
  are code editors.
* Most editors are *value editors*, which means that they are restricted
  to editing constant expressions. For example, a colour editor, if it
  always displays a colour swatch for the colour you are editing,
  is necessarily a value editor.

The set of expressions that can be edited by a domain specific editor
is called a Domain Specific Language (DSL). A DSL will have an associated
grammar that generates all of the expressions that the editor is capable
of editing. For example, a simple RGB colour editor might be restricted to
expressions like `sRGB[r,g,b]`, where the subexpressions `r`, `g` and `b`
are literal constants.

Here are some ideas for DSLs and their editors:
* A noise editor. Maybe like the FastNoiseSIMD preview app:
  <https://github.com/Auburns/FastNoiseSIMD>.
* A fractal editor, like Mandelbulber:
  <https://sourceforge.net/projects/mandelbulber/>.
* A colour editor.
* A 3D scene editor, like TinkerCAD or EditSDF:
  <stephaneginier.com/archive/editSDF/>.
* A 2D scene editor, like Inkscape.
* A spline editor.
* A grammar based shape generator, like <contextfreeart.org>.
* An image editor.

A multiplicity of domain-specific languages and editors will help make
Curv more productive and easier to use. The ability to plug in new DSLs and
new Editors will enable Curv to evolve to become a universal tool for creating
any kind of shape or graphical object.

For a simple image editor like MS Paint, the source code is a pixel array.
In effect, the source code is restricted to being a constant expression.
Adobe Photoshop supports named layers. This is like a let expression that
binds each layer to a variable name, and whose body is an expression that
composites the layers together. There are also procedural image editors that
record the sequence of image operations that you apply, and let you edit this
history, so you are in effect writing a program. All of these types of image
editor can fit in to the Curv projectional editing framework.

Curv expressions are composable. It follows that Editors are composable
(one kind of editor can be embedded in another editor).

Multiple Tiers, from Easy to Powerful
-------------------------------------
Curv is both an easy-to-use end-user programming environment, and it is a
powerful tool for experts. It achieves these goals with tiered APIs.
The highest tiers are easiest to use, at the cost of being more constained
and having fewer degrees of freedom. The lowest tiers are the most powerful,
by virtue of having the most degrees of freedom.

For example, constructing shapes using high level CSG operations is a high
level API. User defined functions (and especially recursion and tail recursion)
is a more advanced feature. Constructing shapes by defining signed distance
functions happens at the lowest tier.

For ease of use, we associate high-level, constrained APIs with high level,
constrained, domain-specific editors.

A high level, easy to use editor doesn't just start with a blank page
that you must fill in from memory. You start with some existing structure
that is already on the screen: controls (API names, parameters, values)
are either initially visible, or can be discovered by unfolding other visible
controls, using direct manipulation.

One example of a high level, constrained, easy-to-use editor is a set of
value picker widgets for editing the parameters of a parameterized shape.

Starting from a high level editor, you can "drill down"
or "open the hood" to expose a lower level and more powerful editor.
In the case of an editor that displays a shape and a set of value pickers,
that would mean opening a code editor on the `parametric` expression that
defines the parametric shape you are editing.

What makes Curv powerful is that it contains a general purpose functional
language with the usual abstraction mechanisms: variables, functions, data
structures, etc. Once you have created a shape using a restricted but high
level editor/DSL, you should always be able to enable the full power of the
Curv language, and generalize your code using abstraction mechanisms. Eg,
abstract out numeric constants and turn them into function parameters.

Interactive Animations
----------------------
One of the goals of the Curv project is to support simple interactive
animations. This feature can be similar to what is expressible in SVG
(which also supports simple interactive animations), or what is expressible
in ShaderToy. Use cases include interactive art projects, and interactive
visualizations embedded in documentation (Wikipedia has a few of these).

These interactive animations can be programmed using Curv. They are a great
way to explore a parameter space of graphical objects, as part of the design
and coding process. Also, I want Curv to have interactive documentation,
partly inspired by Book of Shaders.

A running interactive animation is like an editor session. In both cases,
user interface gestures change the state of a graphical object.

If we treat an interactive animation like an editor session, then we add the
ability to save the state of an animation as "source code".
