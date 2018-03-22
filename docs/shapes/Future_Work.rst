Future Work
===========

SDF Data Structures and Rendering Algorithms
--------------------------------------------
In Curv, shapes are represented by signed distance fields.

In Curv 0.0, SDFs are represented using Function Representation (F-Rep),
and rendered on a display using Sphere Tracing.

However, I want to support a wide range of shape operations:
primitive shapes, shape operators, operations that import and export shapes
from files. In order to support a wider range of shape operations,
I'll need to use some of the alternate data structure representations of SDFs,
not just F-Rep.

Mesh Import
-----------
I want the ability to import an STL file (and other mesh file types like OBJ, AMF, 3MF).
Unfortunately, meshes are probably the worst possible representation for getting geometric data into Curv.
So it won't be easy.
This is a research project, it won't be in the 1.0 release.

There are two use cases: the mesh is an exact representation of the desired shape,
or it is an approximation.

Exact Meshes
  The mesh is an exact representation of a polyhedron; it isn't an
  approximation of a curved shape.
  
  If the polyhedron has only a small number of faces, then you can
  represent it as an intersection and union of half-spaces.
  But the rendering time would be proportional to the number of half-spaces,
  so this approach doesn't scale.
  [Starting point: a Curv function that reads an STL file, returns a list of triangles.]
  
  The polyhedra that appear in math-inspired art tend to be highly symmetrical.
  The best representation of these polyhedra in Curv is as a compact CSG tree
  that explictly encodes all of the symmetries. Automatically converting a mesh to this
  representation is tricky: it would be better to get the original "source code"
  used to generate the mesh file, and port that to Curv.
  
  Alternatively, maybe we can design an efficient data structure for representing
  the distance field of a complex polyhedron?

Approximate Meshes
  The mesh is an approximation to a curved surface.
  
  Sometimes, the mesh is generated as an approximation of
  a more exact digital representation, like an OpenSCAD program, or a parametric
  spline created by a CAD program. In these cases, it would be better
  to convert the original exact representation directly to Curv, bypassing
  the intermediate mesh, since constructing a mesh throws away information
  and adds noise.
  
  In other cases, the mesh is produced by scanning a physical object,
  in which case the mesh is created from a point cloud representation (from a 3D scanner),
  or from a voxel array from a CT scanner or MRI scanner.
  [In the FastRBF paper cited below, it is stated that it's better to start with the original
  point cloud or CT scan data, since the constructed mesh has added noise (extraneous
  vertices).]

  Suppose we have a high-triangle-count approximation to a curved surface,
  like the Yoda bust on Thingiverse (614278 triangles).
  Our best strategy is to convert this into a more compact and efficient representation
  that is an approximation to the polyhedral mesh and reconstructs the curved surfaces
  while preserving edges.
  
  Converting a large mesh to a volumetric format is slow (minutes),
  so I'll use a separate conversion tool and a file format.
  
  Possible requirements:
  
  * Good quality SDF, suitable for sphere tracing.
  * Handles low quality input.
    Triangle meshes are often of poor quality:
    not 2-manifold (not watertight, self intersections);
    zero area triangles; not orientable (some normals point in the wrong direction);
    excessive detail.
  * Compact representation, since it has to fit in GPU memory.
    3D voxel arrays are simple but not compact.
  * Fast SDF evaluation.
    It's likely that Yoda will compile into a large representation.
    If all of the data is accessed each time the Yoda SDF is evaluated,
    then evaluation will be too slow. We'd prefer a compiled representation where only a small fraction
    of the data needs to be accessed when evaluating the SDF at a given point.
    Trees and arrays indexed by geometric location have the right kind of access properties.
  * GPU acceleration.
  
  This has been an active area of research for decades. There are lots of possibilities.
  
  * **3D voxel arrays** are simple and popular. Nothing is faster on a GPU.
    Each grid element contains a distance value, and the distance value at a point
    is reconstructed by interpolation using GPU texture hardware.
    
    They can take up a lot of memory, though. A 128x128x128 grid, with 16 bits per sample,
    is 4MBytes, which is tractable. Doubling the linear resolution grows the memory
    requirements by 8 times.
  
    Use GPU hardware to quickly convert a mesh to a voxel array.
    "Interactive 3D Distance Field Computation using Linear Factorization" [2006].
    http://gamma.cs.unc.edu/GVD/LINFAC/

    `Signed Distance Fields for Polygon Soup Meshes`_ (2014):
    Input is polygon soup. Triangles don't need to be correctly oriented,
    mesh doesn't need to be 2-manifold.
    The output is a voxel array.

  * An **ASDF** (Adaptively sampled Signed Distance Field) is essentially a voxel array
    that is compressed using an octree.
    "Adaptively sampled distance fields: A general representation
    of shape for computer graphics" [Susan Frisken, 2000].
    Antimony uses this representation.
    Evaluating an ASDF on a GPU (a requirement for Curv) requires novel data structures,
    which are not in the original research.
   
    GPU-Accelerated Adaptively Sampled Distance Fields (2008):
    http://hyperfun.org/FHF_Log/Bastos_GPU_ADF_SMI08.pdf
    Input is a 2-manifold mesh, output is an ASDF (adaptively sampled distance field)
    which is then rendered on a GPU using sphere tracing.
  
    Use a GPU to create and then evaluate an ASDF.
    "Exact and Adaptive Signed Distance Fields Computation
    for Rigid and Deformable Models on GPUs" [2014]
    http://graphics.ewha.ac.kr/gADF/gADF.pdf
  
    An hp-ASDF is a more sophisticated ASDF.
    "Hierarchical hp-Adaptive Signed Distance Fields" (2016)
    https://animation.rwth-aachen.de/media/papers/2016-SCA-HPSDF.pdf
  
  * **Radial Basis Functions**
    are a kind of spline representation with an associated distance field.
    
    * Any SDF can be converted to RBF form. This suggests that an expensive SDF described
      using Curv could be converted to an approximate RBF that is cheaper to evaluate.
    * If you convert the resulting RBF back to a mesh, applications include mesh simplification
      and mesh repair.
    * "Gradients and higher derivatives are determined analytically and are continuous and smooth",
      avoiding a problem with discretely sampled SDFs, which tend to be discontinuous across cell boundaries.
    
    "Reconstruction and Representation of 3D Objects with Radial Basis Functions" (2001)
    http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.58.1770&rep=rep1&type=pdf
    
    This is the FastRBF method. It is "difficult to implement".
    It has the limitation that the RBF is "global" [non compactly supported],
    meaning you have to evaluate the entire RBF
    (potentially containing a large number of "centres" or spline points) to query the SDF at any point.
    So SDF evaluation would be slow.
    
    "Implicit Surface Modeling Suitable for Inside/Outside Tests with Radial Basis Functions" (2007)
    http://vr.sdu.edu.cn/~prj/papers/p20071019.pdf
    
    Easier to implement. Uses compactly supported basis functions.
    Produces a more exact distance field (than other methods).
    
    "Modelling and Rendering Large Volume Data with Gaussian Radial Basis Functions" (2007)
    https://www.derek.juba.name/papers/RBFVolume_Tech.pdf
    
    This paper puts the RBF centres into an octree to speed up rendering (on a GPU).
    You can dynamically trade off accuracy for rendering speed by controlling how deep
    you descend the octree.
  
A Hybrid Geometry Engine
   We could abandon the idea of converting a mesh to an SDF.
   Instead, implement a hybrid geometry engine, where some shapes are represented
   as meshes, some are represented as SDFs, and some are hybrid unions of
   meshes and SDFs. Some operations work on all 3 representations (eg,
   affine transformations). Some operations work only on meshes, or only on SDFs.
   You can convert an SDF to a mesh (but not vice versa).
   A top level scene is a union of meshes and SDFs, rendered using some hybrid
   Z-buffer algorithm. But, there are a lot of Curv operations that won't work
   on Yoda, and the whole implementation is twice as complex.

.. _`Signed Distance Fields for Polygon Soup Meshes`: http://run.usc.edu/signedDistanceField/XuBarbicSignedDistanceField2014.pdf

I'll begin by using voxel arrays, since they are the industry standard.
I'll use a separate tool (eg, https://github.com/matejd/DistanceFieldGen) to convert a mesh
to a (compressed) 3D texture SDF, stored in a KTX file. I'll extend Curv to import these KTX files.

Voxel Arrays
------------
A voxel array containing signed distance values is an alternate representation of an SDF.
Interpolation (using GPU texture hardware) is used to compute the distance value at a point.
All of the Curv shape operations will work on this representation.

Benefits:

* Uniformly fast evaluation on a GPU.
* An F-Rep SDF that is too expensive to evaluate during interactive previewing
  can be sped up by conversion to a voxel array.
* Easy and fast to convert a mesh file to a voxel array.
* There are useful shape operators that require a voxel array, not an F-Rep SDF.
  Eg, "Level Set Surface Editing Operators", http://www.museth.org/Ken/Publications_files/Museth-etal_SIG02.pdf
  Or, possibly, some of the ShapeJS operations: http://shapejs.shapeways.com/

The disadvantage is that it is an approximate sampled representation, not an
exact representation. And storage requirements increase with the cube of the resolution.

Convex Hull
-----------
An OpenSCAD operation that is difficult/expensive to implement in F-Rep.
It's a powerful and intuitive operation, so it would be nice to have for that reason alone.

Convex Hull is used to create a skin over elements that form the skeleton of the desired shape.
There are probably better and cheaper ways to accomplish this in F-Rep,
so this operation is not a must-have.

Convex Hull could be implemented in restricted form as a Polytope Operator (see below).
This means it's not supported on curved surfaces.

Convex hull of two copies of the same shape is equivalent to sweeping that shape
over a line segment: there is a separate "TODO" entry for Linear Sweep.

Minkowski Sum
-------------
An OpenSCAD operation that is difficult/expensive to implement in F-Rep.
I personally like Minkowski sum, but there is a learning curve in understanding
how it works. It's not intuitive to people who first encounter it.

The most common Minkowski sum idioms have cheaper direct implementations in F-Rep
which are also easier to understand.

* Rounded offset at distance d: Minkowski sum with a sphere of radius d, or ``offset d``
  of a shape with an exact distance field.
* Shell: in Curv, ``shell``.
* Morph between two shapes: in Curv, ``morph``.
* Sweep a 3D solid along a 3D curve: This has its own entry in the TODO list,
  and might be easier than a general Minkowski sum implementation.

My intuition says that Minkowski sum ought to be implementable as a Nested Distance Field
operation on shapes with exact distance fields, analogous to ``perimeter_extrude``.
But it's not quite as simple as that, and an actual implementation is likely to be expensive.

Splines
-------
Spline support is important for compatibility with external tools that create spline curves and surfaces.
Adobe Illustrator supports cubic Bezier curves only. The SVG file format supports quadradic and cubic Beziers.
Inkscape can read quadratic Beziers, but it elevates them to cubic for editing.
3D CAD programs (FreeCAD, Rhino, etc) additionally support B-Splines and NURBS (and sometimes T-Splines).

* Sweep a spline curve using a circle/sphere in 2D/3D. Open or closed curve.
  A solution for cubic Bezier curves is outlined in `Sphere Tracing`_, based on code from Graphics Gems:
  https://github.com/erich666/GraphicsGems/blob/master/gems/NearestPoint.c.
  
  * Given a point ``p`` and 4 Bezier control points, construct a 5th order Bezier equation
    whose solution finds the point on the curve closest to ``p``.
  * Find the roots of the 5th degree equation using iterative root finding.
    The roots are parameter values ``t`` for the Bezier curve.
  * Evaluate the Bezier at each root ``t`` to produce a set of candidate points.
    Extend the set of candidate points with the first and last control point, which are
    the endpoints of the curve. Select the candidate point that is closest to ``p``.
  * The distance from ``p`` to the candidate point gives an exact SDF for a zero-width Bezier curve.
    Subtract ``d`` from the SDF to sweep the curve with a ball of radius ``d``.

* Construct a shape by filling the space bounded by a closed spline curve (2D)
  or surface (3D).

Mathematica has BezierCurve, BSplineCurve, and BSplineSurface (for NURBS).

Circle/Sphere Sweep of a Parametric Curve
-----------------------------------------
Spline curves are a special case of parametric curves.
There are lots of interesting mathematical art objects defined by parametric equations.
Eg, I'd like to sweep out a `trefoil knot`_ with a sphere,
using the parametric equations::

  x = sin t + 2*sin(2*t)
  y = cos t - 2*cos(2*t)
  z = -sin(3*t)

.. _`trefoil knot`: https://en.wikipedia.org/wiki/Trefoil_knot

This would be trivial if we could analytically convert these parametric equations to implicit form.
I'm not sure there is a general solution to this problem.
According to `Geometric and Solid Modeling`_, chapter 5:

  General techniques exist for converting [algebraic surfaces and curves]
  from parametric to implicit form, at least in principle,
  and we review here a simple version based on the Sylvester resultant. In
  Chapter 7, we show how to use Grobner bases techniques for this purpose.

Lots of useful curves aren't algebraic (ie, polynomial), like the helix,
the sine wave, and the trefoil knot. (Is there a more general solution for
analytic conversion?)

I also know that an analytic solution can be too expensive to use.
In `The Implicitization of a Trefoil Knot`_, Michael Trott
converts the trefoil knot parametric equation to implicit form, using Mathematica.
"The result is a large polynomial.
It is of total degree 24, has 1212 terms and coefficients with up to 23 digits."

.. _`The Implicitization of a Trefoil Knot`: https://www.google.ca/url?sa=t&rct=j&q=&esrc=s&source=web&cd=13&ved=0ahUKEwj9o7-S9tvUAhWl24MKHYjLCwAQFghPMAw&url=http%3A%2F%2Fwww.mathematicaguidebooks.org%2Fscripts%2Fdownload_file.cgi%3Fsoftware_download%3DSample_Section_Symbolics.nb.pdf&usg=AFQjCNHYR408D7qpaYvJC7500ylz9iY0Mw

What about a numerical solution?
According to "Image Swept Volumes" (Winter and Chen),
accurate numerical solutions can often be quite expensive.
(Fine, but let's try it anyway. How do I do that?)

So we are looking for some way to remove the heavy lifting from the trefoil knot SDF distance function.

For example, compile the parametric equations into a data structure that can be efficiently queried
by the distance function to produce a reasonable approximation of the curve.
Sample the parametric curve, either at regular intervals, or adaptively (higher sampling
rate where the curvature is higher). Put the sample values into a balanced space partitioning
tree structure. The distance function looks up the nearest sampled points in the tree
and then:

1. uses polynomial interpolation to estimate the nearest point on the curve.
2. uses root finding to find the value of t for the closest point on the curve.

Either way, we are creating an approximation to the curve, within some error tolerance.
If a non-linear transformation is applied, and part of the curve is scaled to a larger size,
then a smaller error tolerance may be required in the scaled region of the curve.
So let's think about how to dynamically
determine the appropriate error tolerance during SDF evaluation time.

Precompiling the parametric equations to a data structure won't work if the equations
contain coefficients derived from SDF evaluation time data (x,y,z,t coordinates).

.. _`Geometric and Solid Modeling`: https://www.cs.purdue.edu/homes/cmh/distribution/books/geo.html

Linear Sweep
------------
Sweep an arbitrary 2D/3D shape along a 2D/3D line segment.

General Sweep
-------------
Sweep an arbitrary 2D/3D shape along an arbitrary 2D/3D curve.

Libigl (open source) implements this for 3D triangle meshes. It converts the mesh to V-Rep, performs the sweep, then converts V-Rep back to a mesh. Since Curv is already a V-Rep system, we can cut out the conversion between B-Rep and V-Rep. The actual V-Rep used is a discrete signed distance field, represented as a voxel array. See http://libigl.github.io/libigl/tutorial/tutorial.html#sweptvolume

References:

* William J. Schroeder, William E. Lorensen, and Steve Linthicum. "Implicit Modeling of Swept Surfaces and Volumes", 1994.
* Akash Garg, Alec Jacobson, Eitan Grinspun. "Computational Design of Reconfigurables", 2016.
  http://www.cs.columbia.edu/cg/reconfigurables/

General Extrude
---------------
Sweep an arbitrary 2D shape along an arbitrary 3D curve.
The shape is normal to the curve at all points.
A generalization of ``extrude``.

research:

* "Image Swept Volumes", Winter and Chen, 2002, http://vg.swan.ac.uk/vlib/DOWNLOADS/ISV.pdf
* "Implicit Sweep Surfaces", Schmidt and Wyvill, 2005, http://unknownroad.com/publications/SweepsTRApril2005.pdf

Pixelate
--------
Transform a 2D shape so that it appears to be made of uniformly sized and coloured pixels,
or transform a 3D shape to voxels. The goal is to create a common
artistic effect: eg, make a shape look like it was modeled in Minecraft.

Convolution
-----------
A low pass filter would remove high frequency components from a shape,
rounding off sharp vertices and edges, and in effect "blurring" the shape.
Mathematical convolution is a way to implement this.

Local Deformations
------------------
These operations treat a shape as a lump of clay,
in which local regions can be arbitrarily deformed
while leaving the rest of the shape unmodified.
They are found in `digital sculpting`_ programs like ZBrush.

.. _`digital sculpting`: https://en.wikipedia.org/wiki/Digital_sculpting

CorelDraw has Smear, Twirl, Attract and Repel operators,
which perform smooth local translations, rotations and +/- scaling.
This seems like a good starting point.
Antimony has Attract and Repel in open source.

Drawing Text using a Font
-------------------------
Signed distance fields are now considered the best way to render text using a GPU.
For example, the Qt graphics toolkit uses SDFs for text rendering.
This fits into Curv really well.

The trick is to convert each character into a discretely sampled SDF, stored in a texture.
This happens before SDF evaluation time (rendering).
During rendering, we do interpolated texture lookups to get the value of a character SDF at a point.

Conway Polyhedron Operators
---------------------------
Implement the `Conway polyhedron operators`_.
Existing polyhedron constructors like ``cube``, ``icosahedron``, etc, are modified so that they
can be used as input values.

``antiprism n``
  3d shape constructor, one of the Conway primitives.

* In OpenSCAD, by Kit Wallace: https://github.com/KitWallace/openscad/blob/master/conway.scad
* In JavaScript/VRML, by George Hart: http://www.georgehart.com/virtual-polyhedra/conway_notation.html
* HTML5: http://levskaya.github.io/polyhedronisme/

.. _`Conway polyhedron operators`: https://en.wikipedia.org/wiki/Conway_polyhedron_notation

Polytope Operators
------------------
A polytope is either a polygon or a polyhedron.
Polytopes contain additional shape attributes representing the vertices and faces.
Polytope operators are operations that only make sense on polytopes, not on general curved shapes.
They operate directly on the vertices and faces.

* The Conway polyhedron operators are an example, although some of these operators
  may not work on general polyhedra (to be investigated).
* Convex hull is possibly another example. It's a standard operation on polyhedral meshes,
  but I don't have an implementation for SDFs.
* The boolean operators and affine transformations take arbitrary shapes as arguments (including polytopes)
  but do not return polytopes as results. We could generalize these operators to return polytopes, when given
  polytopes as input. Note that ``union`` is very cheap, and ``polytope_union`` is very expensive, and also
  numerically unstable (fails for some valid inputs).
* ``polygonize`` maps an arbitrary shape to a polytope that approximates the shape.

Supershapes
-----------
Superquadrics were popularized by Alan Barr as a solid modelling primitive.
Includes superellipsoids, superhyperboloids, supertoroids.
See `Sphere Tracing`_ for distance functions.

Supershapes, constructed using the Superformula, are a generalization of Superquadrics.
Implicit function representation for supershapes: http://le2i.cnrs.fr/IMG/publications/PG05.pdf

Hypertextures
-------------
Using Perlin noise (fractal noise) to deform a shape.
See `Sphere Tracing`_.

Fractals
--------
Mandelbulber uses SDFs and sphere tracing to render fractals
constructed using a variety of algorithms.
These algorithms could be packaged as Curv shape constructors.

http://mandelbulber.com/

"At the Heart of the Holy Box" -- I'd like to be able to do this in Curv, in real time on the GPU. Should eventually be possible. (It's just sphere tracing!) https://www.youtube.com/watch?v=OW5RnrlTeow

Voronoi Diagrams
----------------
In 2 and 3 dimensions, are a popular modeling technique in 3D printed geometric art.

Hyperbolic Geometry
-------------------
The math behind Escher's Circle Limit prints.

Check out the animations in this slide show. I'd like to program these in Curv:

* "Conformal Models of the Hyperbolic Geometry", Vladimir Bulatov.
  http://bulatov.org/math/1001/

Carole Blanc
------------
* http://dept-info.labri.fr/~blanc/abst.html

A number of these papers are directed towards the creation of a Curv-like
geometry system.

Spirals
-------
``log_spiral ...``
  TODO: logarithmic spiral

``linear_spiral ...``
  TODO: linear (aka Archimedean) spiral

``repeat_spiral ... shape``
