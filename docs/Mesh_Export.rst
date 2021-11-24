Mesh Export
===========

To export a 3D shape to an STL, OBJ or X3D file, use::

   curv -o foo.stl foo.curv
   curv -o foo.obj foo.curv
   curv -o foo.x3d foo.curv

File Formats
------------
Which format should you use?

* STL is the most popular format for 3D printed objects.
  It's the only format in this list understood by OpenSCAD.
* OBJ is the recommended format for export from Curv (unless you need colour
  or OpenSCAD import).

    * It is supported by all 3D printing slicer software and service providers.
    * The files are significantly smaller (they can be 20% of the size of an STL
      file).
    * OBJ files record "topology" information, which is needed by some
      applications. Meshlab imports OBJ files with no issues, whereas it needs to
      repair an STL file when it imports it, and I've seen Meshlab hang up while
      attempting to do this (for a large file).

* X3D contains colour information. Use it for full colour 3D printing on
  shapeways.com, i.materialise.com, etc.

Mesh export provides a way to visualize models that are not compatible
with the viewer (because their distance function is too slow or not
Lipschitz-continuous). There are examples in `<../examples/mesh_only>`_.

Mesh Generators
---------------
There are two mesh generating algorithms, called ``#smooth`` and ``#sharp``,
with complementary strengths and weaknesses.
They are specified on the command line like this::

    curv -Omgen=#smooth
    curv -Omgen=#sharp

The ``#smooth`` algorithm is recommended for smooth, organic looking shapes,
and especially for fractals. It is the fallback algorithm when ``#sharp``
doesn't work. It mostly generates quads, which makes it good
for computer graphics models that are imported into mesh editors like Z-Brush.
The output is guaranteed to be topologically correct (watertight, manifold,
no self intersections). However, it does not preserve sharp features such as
edges and corners (instead, rounding them off).

The ``#sharp`` algorithm preserves sharp features, and is recommended for
CAD-like models with exact or mitred distance fields, made out of primitives
like ``cube``, ``sphere`` and ``cylinder``, combined using boolean operations
like ``union`` and ``intersection``, transformed with similarity transformations
(``move``, ``rotate``, ``reflect``, ``scale``). It has a built in mesh
simplifier that produces fewer faces than ``#smooth``. For the subset of models
that work well in ``#sharp``, the output is excellent.

However, certain transformations (such as ``bend`` and ``twist``) result in
"bent" or "twisted" distance fields that may cause the ``#sharp`` algorithm to
misbehave and create spiky artifacts. The ``#sharp`` algorithm requires surface
areas (other than edges) to have smoothly varying normals, so it can't be used
with pure fractal shapes, which don't have normals. The algorithm is not
guaranteed to create a topologically correct mesh. If you get a bad mesh, you
can't import it into OpenSCAD, you may have problems 3D printing it, and it may
look bad in a 3D CG application (like a game) due to bad face normals.

In these cases, you can fall back to ``#smooth``. You can bring back sharp
details by increasing the triangle count (decreasing ``vsize``) and then
simplifying the mesh, as described in the following sections.

The current default is ``#smooth``.

Tweaking parameters to get a better mesh
----------------------------------------
When you convert a Curv shape to a mesh, you are creating an approximation
of the original shape. Meshes cannot exactly represent curved surfaces.
To get a more exact approximation, you can increase the number of triangles,
but at some point the mesh gets too large to be processed.
1-2 million triangles is a practical upper limit for 3D printing,
and home computers may have a smaller upper limit than this.

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

   curv -o foo.obj -O vsize=.1 foo.curv

The ``vcount`` Parameter
------------------------
If ``vsize`` is not specified, then a voxel size will be chosen for you
using the ``vcount`` parameter (approximate voxel count),
which defaults to 100,000.

The ``vcount`` is a measure of the cost of generating a mesh.
It's roughly the number of voxels, so it's proportional to memory consumption,
CPU usage, and number of triangles generated. Depending on how powerful
your machine is (and on how long you want to wait when generating a mesh
when vsize is not specified), you might want the default value of ``vcount``
to be higher or lower. So you can override the default value of 100,000
in your Curv configuration file.

Speeding up Mesh Export
-----------------------
Use ``-O jit`` to make mesh export run 30 times faster.
This compiles the shape into C++ code, then compiles the
C++ code using the ``c++`` command, which must exist in the ``PATH``.

You need the following software installed for this to work.
If you followed the BUILD instructions, it will already be installed.

 * Either the GNU g++ or the clang C++ compiler.
 * The ``glm`` library.

Simplifying the Mesh
--------------------
Suppose you have too many triangles (maybe, it won't 3D print), and you
can't increase the voxel size any further. Then what do you do?

I recommend an external tool, `MeshLab`_, to simplify the mesh:

* Use ``File`` >> ``Import Mesh...`` to load the mesh file.
* Use ``Filters`` >> ``Remeshing, Simplification and Reconstruction``
  >> ``Quadric Edge Collapse Decimation``.
* A dialog box pops up. What works for me is to type a number into the
  ``Percentage reduction (0..1)`` box, such as ``0.5`` or ``0.25``,
  leave the other parameters alone, then click ``Apply``.
  (The ``Planar Simplification`` option helps if you have large flat regions.)
* Use ``File`` >> ``Export Mesh As...`` to save the simplified mesh
  in another file.
  When the ``Choose Saving Options`` appears, you can just select ``None``.

.. _`MeshLab`: http://www.meshlab.net/

..
  Currently, Curv provides an experimental parameter called ``adaptive``.
  If you use ``-O adaptive``, then it reduces the triangle count, at the
  expense of introducing defects in the mesh (self intersection).
  Depending on which software is reading the mesh, self intersections might
  be okay. (The output is worse than MeshLab simplification and less controllable.)

Mesh Quality
------------
The ``#smooth`` algorithm generates watertight, manifold meshes with no self
intersections, degenerate triangles, or flipped triangles. These are high
quality, defect free meshes that can be processed by any software.

* `OpenSCAD`_ requires defect free meshes (otherwise boolean operations fail).
* Meshs submitted to `Shapeways.com`_ for 3D printing *should* be defect free.
  They can automatically repair self intersection (and perhaps other defects),
  but the repair is not guaranteed to succeed, and becomes more likely to
  fail with very large meshes (the upper limit is 2M triangles as of April 2018).

The mesh simplification performed by MeshLab may introduce self-intersections.
This doesn't usually cause a problem for 3D printing, because slicing software
attempts to repair bad meshes.

.. _`OpenSCAD`: http://www.openscad.org/
.. _`ShapeWays.com`: https://shapeways.com/

The tradeoff for defect free meshes is the lack of sharp feature detection.
The edges of cubes are rounded off. To fix this, decrease the
``vsize`` parameter until the rounding effect is no longer objectionable,
then use MeshLab to simplify the mesh.
It's not a perfect solution: you still don't get sharp edges and corners,
and you'll have more triangles than necessary.

Full Colour Meshes
------------------
To create a full colour mesh, export an X3D file.
Use `-O colouring=#face` to give a uniform colour to each face.
Use `-O colouring=#vertex` to colour each vertex (and the vertex colours
will be interpolated across the faces).

Use MeshLab to view the X3D files.

For example::
  curv -o twistor.x3d -O colouring=#vertex -O vsize=0.05 examples/twistor.curv
