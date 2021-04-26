The Curv Shape Library
======================
The shape library is a high level interface for constructing
coloured, animated geometric shapes in 2 and 3 dimensions.
It is designed both for ease of use and for expressive power.

The shape library provides a set of graphical "building blocks",
which can be plugged together in many different ways to construct shapes.
These building blocks comprise: constructors, for making new graphical values;
transformations, for transforming one graphical value into another; and
operators for combining several graphical values to make a new graphical value.
Each building block has an intuitive, visual interpretation which can be explored
in the graphics window. At a deeper level,
there is also a mathematical interpretation, and by understanding the math
you can learn to create new building blocks.

The shape library is built in to Curv; you don't need to explicitly include
an external library. The library is written entirely in Curv; you can read
the source code to learn how to create new blocks.

Graphical Data Types
--------------------
The Curv shape library has the following graphical data types:

shape
  A 2D or 3D geometric shape.

  * `Shapes`_
  * `Shape Constructors`_
  * `Boolean Operations`_
  * `Transformations`_
  * `2D to 3D`_
  * `3D to 2D`_
  * `Repetition`_
  * `Distance Field Operations`_

texture
  A texture is an infinite, possibly animated colour pattern
  that can be applied to a shape, giving it colour.
  Textures can be 2D or 3D.
  They can be constructed by composing intensity fields and colour maps.

  * `Textures`_
  * `Transformations`_
  * `Repetition`_

intensity field
  An intensity field is a colour pattern with the colours abstracted away,
  leaving only a pattern of intensity values, which are numbers between 0
  and 1. Intensity fields can be animated.
  Mathematically, a function that maps [x,y,z,t] onto an intensity,
  where [x,y,z] is a point in space, and t is time.

  * `Intensity Fields`_

colour map
  A continuous range of colours, which can be applied to an intensity field to
  construct a texture.
  Mathematically,
  a function that maps an intensity value between 0 and 1 onto a colour.

  * `Colour Maps`_

colour
  A colour value.

  * `Colour`_

Other Topics
------------
* `Debug`_
* `Future Work`_
* `Bibliography`_

.. _`2D to 3D`: 2D_to_3D.rst
.. _`3D to 2D`: 3D_to_2D.rst
.. _`Bibliography`: Bibliography.rst
.. _`Boolean Operations`: Boolean.rst
.. _`Colour`: Colour.rst
.. _`Colour Maps`: Colour_Maps.rst
.. _`Debug`: Debug.rst
.. _`Distance Field Operations`: Distance_Field_Operations.rst
.. _`Future Work`: Future_Work.rst
.. _`Intensity Fields`: Intensity_Fields.rst
.. _`Repetition`: Repetition.rst
.. _`Shape Constructors`: Shape_Constructors.rst
.. _`Shapes`: Shapes.rst
.. _`Textures`: Textures.rst
.. _`Transformations`: Transformations.rst
