Mesh Export
===========

To export a 3D shape to an STL, OBJ or X3D file, use::

   curv -o stl foo.curv >foo.stl
   curv -o obj foo.curv >foo.obj
   curv -o x3d foo.curv >foo.x3d

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

   curv -o obj -O vsize=.1 foo.curv >foo.obj

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
Curv generates watertight, manifold meshes with no self intersections,
degenerate triangles, or flipped triangles. These are high quality, defect free
meshes that can be processed by any software.

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

Curv does not yet support sharp feature detection,
so the edges of cubes are rounded off. To fix this, decrease the
``vsize`` parameter until the rounding effect is no longer objectionable,
then use MeshLab to simplify the mesh.
It's not a perfect solution: you still don't get sharp edges and corners,
and you'll have more triangles than necessary.

Full Colour Meshes
------------------
To create a full colour mesh, export an X3D file.
Use `-O colour=face` to give a uniform colour to each face.
Use `-O colour=vertex` to colour each vertex (and the vertex colours
will be interpolated across the faces).

Use MeshLab to view the X3D files.

For example::
  curv -o x3d -O colour=vertex -O vsize=0.05 examples/twistor.curv >twistor.x3d
