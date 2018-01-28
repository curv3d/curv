Repetition
==========
The union operator is slow. The cost of a union is equal to slightly more than the sum of the costs of the argument shapes. So if you have a shape that takes 1ms to render, and you union together 1000 copies of this shape, well now it takes 1s to render.

Fortunately, Curv has repetition operators which union together an arbitrary number of copies of a shape together, or even an infinite number of copies, in constant time and space.

Each repetition operator corresponds to a different mathematical symmetry. The most basic ones are:

* Mirror symmetry: Reflect a shape through a plane, giving two copies (the original shape and the mirror image).
* Translational symmetry: Partition space into multiple cells, like a linear array or grid pattern, causing a copy of the shape to appear in each cell.
* Rotational symmetry: Partition space into radial pie slices, causing a copy of the shape to appear in each slice.

Operations
----------
This interface is still experimental.

``repeat_x d shape``
  The original cell to be copied is an infinite strip of width ``d`` along the X axis,
  centred on the origin. This cell is repeated endlessly along the X axis in both directions.
  The shape may be 2D or 3D.

``repeat_xy (dx,dy) shape``
  The original cell to be copied has a rectangular cross section, with width and height (dx,dy),
  centred on the origin, and infinite along the Z axis.
  This cell is repeated endlessly across the XY plane.
  The shape may be 2D or 3D.

``repeat_xyz (dx,dy,dz) shape``
  The original cell to be copied is a cuboid with dimensions (dx,dy,dz),
  centred on the origin.
  This cell is repeated endlessly across 3D space.
  The shape must be 3D.

``repeat_mirror_x shape``
  The contents of the positive X half-space
  are reflected through the YZ-plane
  to make a mirror copy in the negative X half-space.
  The shape may be 2D or 3D.

``repeat_radial n shape``
  Partition space into *n* radial pie slices, radiating from the Z axis.
  "Slice 0" is below the origin, centred on the negative Y axis.
  The contents of "slice 0" are copied into the other *n-1* slices.
  The shape may be 2D or 3D.

Future Work
-----------
* Support more symmetries. For example, consider the `Wallpaper Symmetries`_, the `Frieze Symmetries`_,
  and the book "The Symmetries of Things".
* Make it easy to construct complex symmetries by composing simpler symmetries.
* Permit the `shape` argument of each repetition operator to be optionally replaced by a function
  that maps a cell index onto a shape. This function can customize the contents of each cell or tile
  based on the cell index.
* ``repeat_helix ... shape``

.. _`Wallpaper Symmetries`: https://en.wikipedia.org/wiki/Wallpaper_group
.. _`Frieze Symmetries`: https://en.wikipedia.org/wiki/Frieze_group
