History and Rationale
=====================
Curv builds on earlier work:

Haskell (1998)
  Shows that pure functional programming has awesome
  benefits, and is a viable alternative to imperative style programming.

  *BUT*: Haskell has a very high learning curve, and maybe that's why
  pure functional programming hasn't become more mainstream.
  Someone should build an easy to use pure functional language
  for beginners, with a simple dynamic type system.

Processing (2001)
  A compelling programming language for beginners:
  learn how to program by writing code that makes art.
  A simple GUI with code on the left, the picture it generates on the right.

  *BUT*: Processing is an old fashioned, low level, imperative language.
  It's a 1990's vision of how to write graphical code.

OpenSCAD (2010)
  Similar to Processing, but with a nicer language:
  it's a high level declarative language that manipulates 2D and 3D shapes
  as objects, using powerful operations to transform and combine shapes.
  And, you can export shapes for 3D printing.

  *BUT*: It's slow, and lacks expressive power.
  
  * The geometry engine is based on polyhedral meshes: it's slow and memory
    intensive. Complex models with many triangles are expensive.
    Boolean operations are expensive. Doesn't use the GPU for geometry
    computations.
  * The language lacks expressive power. Functions and shapes are not first
    class values. You can't query the properties of a shape. Very little
    support for colour.

ImplicitCAD (2012)
  A clone of OpenSCAD that uses F-Rep (Function
  Representation) instead of meshes to represent shapes. It provides a glimpse
  of the greater expressive power of F-Rep. Functions are values, so you
  can do functional programming.

  *BUT*: Still a very limited language. You can't define new
  function representations within the language, so most of the potential
  of F-Rep geometry is inaccessible. And it doesn't use the GPU to speed up
  geometry processing.

ShaderToy.com (2013)
  A website with a Processing-like interface
  for doing F-Rep programming using WebGL and the GLSL language. Showcases the
  full power of F-Rep programming on a GPU.

  *BUT*: GLSL is a very low level and limited language. The programming
  model is very difficult. You can't 3D print the models you see on ShaderToy.

Curv combines the best features of its predecessors. It's a high level,
pure functional language for geometric art and 3D printing.
It is easy for beginners to get started with, but it also exposes the full
power of F-Rep programming to experts. Experts can package techniques seen on
ShaderToy as high level operations with a simple API, so that beginners can
create the kind of effects seen on ShaderToy using much shorter, simpler
programs. And you can 3D print your models. All rendering is GPU accelerated.
