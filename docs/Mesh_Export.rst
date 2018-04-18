Mesh Export
===========

To export a 3D shape to an STL, OBJ or X3D file, use::

   curv -o stl foo.curv >foo.stl
   curv -o obj foo.curv >foo.obj
   curv -o x3d foo.curv >foo.x3d

Which format should you use?

* STL is the most popular format for 3D printed objects.
* OBJ is the #2 format, supported by all of the 3D printer slicers
  that I've surveyed. The files are significantly smaller
  (they can be 20% of the size of an STL file).
* X3D contains colour information. Use it for full colour 3D printing on shapeways.com,
  i.materialise.com, etc.

Mesh export provides a way to visualize models whose distance function
is not Lipschitz-continuous. These are typically algebraic implicit
functions. There are examples in `<../examples/mesh_only>`_.

Tweaking parameters to get a better mesh
----------------------------------------
When you convert a Curv shape to a mesh, you are creating an approximation
of the original shape. Meshes cannot exactly represent curved surfaces.
To get a more exact approximation, you can increase the number of triangles,
but at some point the mesh gets too large to be processed.

As a result, mesh export is an iterative, interactive process
where you try different values of the mesh export parameters until you
find the right tradeoff between quality and mesh size. (If the mesh is too
big, you won't be able to 3D print it. If the mesh is too small, it won't
look right.)

The fundamental mesh export parameter is ``vsize``, short for 'voxel size'.
As ``vsize`` becomes smaller, you get more triangles, and the output becomes
more accurate. As a rule of thumb, ``vsize`` should be half the size of the
smallest detail you want to capture, and half the thickness of the thinnest
wall. If ``vsize`` is too large, small details will disappear and holes will
appear in thin walls.

You set the ``vsize`` parameter using ``-O vsize=N``. For example::

   curv -o obj -O vsize=.1 foo.curv >foo.obj

Simplifying the Mesh
--------------------
Suppose you have too many triangles (maybe, it won't 3D print), and you
can't increase the voxel size any further. Then what do you do?

I recommend an external tool, ``meshlab``, to simplify the mesh:

* Use ``File`` >> ``Import Mesh...`` to load the mesh file.
* Use ``Filters`` >> ``Remeshing, Simplification and Reconstruction``
  >> ``Quadric Edge Collapse Decimation``.
* A dialog box pops up. What works for me is to type a number into the
  ``Percentage reduction (0..1)`` box, such as ``0.5`` or ``0.25``,
  leave the other parameters alone, then click ``Apply``.
  (The ``Planar Simplification`` option can help if you have large flat regions.)
* Use ``File`` >> ``Export Mesh As...`` to save the simplified mesh
  in another file.
  When the ``Choose Saving Options`` appears, you can just select ``None``.

..
  Currently, Curv provides an experimental parameter called ``adaptive``.
  If you use ``-O adaptive``, then it reduces the triangle count, at the
  expense of introducing defects in the mesh (self intersection).
  Depending on which software is reading the mesh, self intersections might
  be okay.

Mesh Quality
------------
By default, Curv generates watertight, manifold meshes with no self
intersections. These are high quality, defect free meshes that can be
processed by any software.

Unfortunately, Curv does not currently support sharp feature detection,
so the edges of cubes are rounded off. What you do instead is decrease the
``vsize`` parameter until the rounding effect is no longer objectionable,
then use ``meshlab`` to simplify the mesh.

Future Work
-----------
The most popular meshing algorithm with sharp feature detection
is Dual Contouring. Unfortunately, it creates defective meshes
(self intersections, not watertight, not manifold).
Still, I might add support for Dual Contouring as an option,
until I find The One True Meshing Algorithm.

An ideal meshing algorithm has these features:

* It produces a defect free mesh.
* It performs sharp feature detection, so that the edges and corners
  of a cube are sharp.
* It is adaptive. It uses lots of small triangles in regions of high
  curvature. It uses fewer larger triangles in areas of low curvature.
  Flat surfaces are rendered with a minimum number of triangles (eg,
  a cube is rendered as 6 quads or 12 triangles,
  regardless of the voxel size).
* It avoids generating needle shaped triangles, which can become degenerate
  in the limiting case.
* There is only one parameter, an error tolerance.

More ideas for future work:

* Speed up mesh generation.
* Provide an integrated, interactive GUI for tweaking the mesh generation
  parameters until you get a good result.
* Provide a way to store machine-readable meshing parameters in a shape.
* Provide a way to mark a shape as mesh-only (it can't be visualized using
  the standard sphere-tracing viewer, because the distance function is
  not Lipschitz-continuous). The curv command will automatically preview
  these shapes by constructing a mesh.
