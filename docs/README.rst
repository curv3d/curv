1. The User Interface
=====================

The user interface is the ``curv`` command, which you run from a Terminal
window using a command line shell like ``bash``. After building and installing
``curv``, use ``curv --help`` for command line options.

**Batch mode**

To view a shape defined by a Curv program without editing the file,
just type ``curv myshape.curv``. For example::

  $ curv examples/shreks_donut.curv
  3D shape 75.3982×75.3982×25.1327
  **a graphics window opens, displaying the shape**

**Live Editing Mode (**\ ``curv -le``\ **)**

Type ``curv -le myshape.curv`` to create a 3 window GUI for live editing
a Curv script.

* A text editor window pops up, for editing ``myshape.curv``.
  (The file doesn't need to exist yet.)
* A graphics window pops up either immediately, or once ``myshape.curv``
  exists and contains a valid Curv program.
* The original Terminal window that you invoked ``curv -le`` from
  now serves as the console window. This is where error messages are
  displayed.

After repositioning the text editor, graphics window and Terminal window
so that they are all visible, you can now start editing your Curv script.
Each time you save the file, the graphics window updates to display the
new shape.

(Type Ctrl-S in Linux or Command-S in macOS to save the file.)

When you close the text editor window, the graphics window
disappears and the ``curv`` command exits, giving you a new shell prompt.
[On macOS, you must quit the ``gedit`` text editing application (eg, Command-Q)
before the ``curv`` command will exit.]

**Interactive CLI mode**::

  $ curv
  curv> 2+2
  4
  curv> cube
  3D shape 2×2×2
  **a graphics window opens, displaying the shape**
  curv>

**Exporting a mesh (STL, OBJ or X3D file)**

To convert a shape into an STL file for 3D printing, use::

  $ curv -o foo.stl foo.curv

For more details, see `<Mesh_Export.rst>`_.

**Exporting an image (PNG format)**

To export a 2D shape as a PNG image file, use::

  $ curv -o foo.png foo.curv

For more details, see `<Image_Export.rst>`_.

**Configuration File**

You can customize the behaviour of the Viewer window
and of file export (``-o``) using command line ``-O`` options.
Use ``curv --help`` for help.
You can create a configuration file to specify default values
for the ``-O`` options, and thereby customize the behaviour of Curv
when viewing or exporting shapes.

For more details, see `<Config.rst>`_.

..
  **Live Programming Mode (**\ ``curv -l``\ **)**:

  This is a mode where you have a 3 window GUI, similar to live programming
  in the OpenSCAD GUI. The 3 windows are: the editing window, the graphics window,
  and the console window (which displays error messages).

  * Open a text editor window, editing ``myshape.curv``.
  * Open a terminal window and run ``curv -l myshape.curv`` from ``bash``.
  * Each time you save changes to ``myshape.curv``, the file will be re-evaluated
    and the new shape will be displayed in a graphics window.
  * Keep the terminal window visible: if there are errors in your Curv program,
    they will be displayed here.

  **Live Editing Mode (**\ ``curv -le``\ **)**:

  This is a more convenient way to start up a 3 window GUI.
  You just type ``curv -le myshape.curv``. A text editor window pops up.
  A graphics window pops up either immediately, or once ``myshape.curv`` exists
  and contains a valid Curv program.
  The original terminal window that you invoked ``curv -le`` from now serves as
  the console window. When you close the text editor window, the graphics window
  disappears and the ``curv`` command terminates.

  In order to make this work, you need to set the environment variable ``CURV_EDITOR``
  to a command that takes a filename argument and opens a text editing window.
  This command must run in the foreground, and not exit until you close the text editing window.
  Not all text editors can be run this way. For example,

  * ``export CURV_EDITOR=vim`` will not work, because ``vim`` will run in the terminal
    window, and will not open a separate text editing window.
  * ``export CURV_EDITOR=gvim`` will not work, because by default, the ``gvim`` command
    forks a new process to run the text editor window in, then exits almost immediately.
  * ``export CURV_EDITOR="gvim -f"`` works. The ``-f`` flag forces ``gvim``
    to run in the foreground.

  So, you can add ``export CURV_EDITOR="gvim -f"`` (substituting your favourite text editor)
  to your bash ``.profile`` file in your home directory, and then ``curv -le filename``
  will just work.

2. The Core Language
====================
Full language documentation: `<language/README.rst>`_.

Curv is a *domain specific language* for constructing 2D and 3D
geometric shapes. It's not a general purpose programming language.
There is no concept of I/O. A Curv program is simply an expression that
computes a shape, without causing side effects.

Curv is *dynamically typed*, with 6 fundamental types:
symbols, numbers, strings, lists, records and functions.
All application level data is built from these 6 types.
For example, a geometric shape is simply a record value
with a standard set of fields.

Curv is an *array language*: scalar arithmetic operations are generalized
to work on vectors, matrices, and higher dimensional arrays. A vector is
represented as a list of numbers: the syntax is ``[x,y,z]``.
A matrix is a list of vectors.

Curv is a *pure functional language*. This means that functions are values,
and that functions are "pure": they compute a result value that varies only
based on the argument values. There is no global mutable state
that a function can read.

Function call (``f x``) is a binary operation that applies the function ``f``
to the argument ``x``. There are 3 ways to pass more than one argument
to a function:

Argument lists:
  ``f[x,y]`` can be thought of in two ways: as passing a single argument
  (a 2-list) to the function ``f``, or as passing 2 arguments.
Argument records:
  ``rotate{angle: a, axis: v}`` is a function call where the argument is a
  record value, which can be interpreted as a function call with multiple
  named arguments.
Curried functions:
  ``f x y`` is a function call ``f x``, returning another function that is
  applied to ``y``. This can be interpreted as a call to a "Curried" function
  with two arguments.

A nested function call like ``f(g(h x))``
can be rewritten as ``x >> h >> g >> f``, which reads like a Unix pipeline,
with data passing from left to right. When combined with curried functions,
this syntax is used for chaining together geometry operations without
nested parentheses. For example::

  sphere 1 >> colour red >> move[10,0,0]

A *let block* allows locally scoped definitions to be included in an expression::

  let definition1; definition2; ... in result_expression
  
Here are examples of definition syntax::

  pi = 3.141592653589793;
  shape = cube >> colour red;
  factorial n = product(1..n);

Within a let block, the scope of each definition is the entire block,
and function definitions may be recursive or mutually recursive.
The order of definitions does not matter.

Curv is an *expression language*: programs are expressions, blocks are expressions,
``if (cond) a else b`` is an expression, etc.
Consequently, every syntactic construct can be nested in every other construct.

..
  Curv programs are stored in ``*.curv`` files.
  A Curv program is an expression that computes a value.
  A typical Curv program computes a shape

3. The Shape Library
====================
Full shape library documentation: `<shapes/README.rst>`_.

The "shape library" is a set of primitive shapes,
and a set of operations for transforming and combining shapes to create
new shapes.

The goal is for this interface to be both powerful and easy to use.

* It's powerful because there is a rich collection of powerful operations
  that can be combined in many different ways.
* Building new shapes is as easy as plugging together existing shapes and
  operators like Lego.

You can look in `<../examples>`_ to see examples.

The low level interface used to implement the shape library is
poorly documented right now.
You can read the source code in `<../lib/curv/std.curv>`_.
See `<Theory.rst>`_ more more information about how the shape library works.
