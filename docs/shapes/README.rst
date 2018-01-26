The Curv Shape Library
======================
The shape library is a high level interface for constructing
coloured, animated geometric shapes in 2 and 3 dimensions.

The shape library is designed for ease of use and expressive power.
It provides a set of "building blocks", each of which has a simple
visual interpretation. The blocks can be combined in a large number of ways
to construct new shapes.

The shape library is built in to Curv; you don't need to explicitly include
an external library. The library is written entirely in Curv; you can read
the source code to learn how to create new blocks.

Data Types
----------
The Curv shape library has the following data types:

shape
  A 2D or 3D geometric shape.

texture
  A texture is an infinite, possibly animated colour pattern
  that can be applied to a shape, giving it colour.
  Textures can be constructed by composing intensity fields and colour maps.

intensity field
  An intensity field is a colour pattern with the colours abstracted away,
  leaving only a pattern of intensity values, which are numbers between 0
  and 1. Intensity fields can be animated.
  Mathematically, a function that maps (x,y,z,t) onto an intensity,
  where (x,y,z) is a point in space, and t is time.

colour map
  A sequence of colours, which can be applied to an intensity field to
  construct a texture.
  Mathematically,
  a function that maps an intensity value between 0 and 1 onto a colour.

colour
  A colour value.

colour space
  Used to construct colour values.
