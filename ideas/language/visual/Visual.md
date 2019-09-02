Visual Curv
===========
Visual Curv is a dialect of Curv that supports visual programming.

Graphical Values
----------------
In Classic Curv, there are 6 fundamental data types (symbol, number, string,
list, record and function), and all values are made from these types.
All values have a textual presentation. There are 6 such representations,
correponding to the 6 types: `#foo`, `42`, `"foo"`, `[]`, `{}`, `<function>`.

In Visual Curv, we want to go beyond the textual presentations of values.
There will be a large repertoire of values that have a graphical, rather
than a textual, presentation when they are displayed:
* shapes
* 2D images (loaded from a PNG file, or procedurally generated)
* 3D voxel grids (loaded from a file, or procedurally generated)
* colours
* colour maps
* intensity maps
* colour fields
* materials (for physically based rendering of 3D graphics)

In Classic Curv, a colour is represented by an RGB triple: [r,g,b].
Colours are represented by the same type used to represent 3D points and 3D
vectors.

In Visual Curv, we want a colour to be presented as a colour swatch,
because that is a higher level, more user friendly presentation.
For example, in the REPL, if you type `red` and hit enter, you will see
a red colour swatch, instead of just the vector `[1,0,0]`.
If you display a list of colours, you will see a list of colour swatches.

From the user's perspective, "Colour" is now an abstract data type, disinct
from a 3 element vector. This means: you must call a constructor function
to construct a colour, (you can't just use a `[...]` list or `{...}` record
literal), and when you print or display the value, you will see a high level
graphical presentation, not just a list or record literal.

So Visual Curv now has a collection of abstract data types with graphical
presentations. The actual, internal representation of these "abstract data
types" is still to be decided.

Perhaps there is some mechanism for "plugging in" new graphical value types
(see section on Plugins).

Graphical Editors
-----------------
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
  <https://stephaneginier.com/archive/editSDF/>.
* A 2D scene editor, like Inkscape.
* A spline editor.
* A grammar based shape generator, like <https://contextfreeart.org>.
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

Plugins and Packages
--------------------
I've mentioned the idea of being able to add "plugins" which define
* new graphical value types,
* new DSLs and new editors,

rather than hardwiring these things into the Curv implementation.

Now, I also want to add a package system to Curv, so that Curv programs
can reference external dependencies using URLs. A built-in package manager
will automatically load dependencies, so it is very easy to reference and
use somebody elses code. You don't have to manage all of the other project's
dependencies by hand.

These so called "plugins" are dependencies that need to be managed
automatically by the package manager. Whatever a plugin is, it needs to have
a platform independent representation.

I can think of 3 possible representations:
* Curv -- perhaps at least some plugins are just Curv code. For example,
  perhaps new graphical value types can be defined directly in Curv.
* WebAssembly modules.
* Source code written in some general purpose programming language that is
  more powerful than Curv. The desktop and web implementations of Curv
  will need to embed a compiler/interpreter for this language. Perhaps it
  is Javascript (that makes sense for the web), and perhaps the desktop
  implementation of Visual Curv is an electron app.

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
visualizations embedded in documentation (Wikipedia has a few of these,
or see Explorable Explanations).

These interactive animations can be programmed using Curv. They are a great
way to explore a parameter space of graphical objects, as part of the design
and coding process. Also, I want Curv to have interactive documentation,
partly inspired by Book of Shaders.

A running interactive animation is like an editor session. In both cases,
user interface gestures change the state of a graphical object.

If we treat an interactive animation like an editor session, then we add the
ability to save the state of an animation as "source code".

Commentary / Literate Programming
---------------------------------
In Classic Curv, comments are restricted to ASCII text, and they are equivalent
to white space (they do not appear in the grammar or in the syntax tree).

In Visual Curv, commentary is part of the syntax of the language. Comments
appear in the abstract syntax tree, so they can be viewed and edited by
a projectional code editor.

Also, Visual Curv comments are not restricted to being presented as fixed
width ASCII. Instead, we support structured text, perhaps equivalent to
Markdown. Plus, you can embed graphical values in commentary, including
images, and interactive animations written in Curv.

File Format
-----------
The file format for Curv source code doesn't change much.
It's still a human readable and editable, text based programming language
syntax. We aren't switching to XML or binary. I want Curv source files to
continue to be compatible with old fashioned text editors and text based
programming practices.

The syntax for comments will change. We now need to represent structured text.
So let's use Markdown.

CML (Curv Markup Language) is Pandoc Markdown, with extensions for embedding
Curv code, and for embedding graphical values as Curv constant expressions.

I would like to redo the Curv documentation in the style of The Book of Shaders,
with embedded Curv examples that you can modify, and with embedded interactive
animations (Explorable Explanations). This documentation will be written in CML.

CML is also the syntax for the text of comments in `*.curv` files.
