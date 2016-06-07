# Geometric Shapes

Shapes are represented by objects.

predicates:
* is_shape -- test if value is a shape; see below
* is_2dshape, is_3dshape -- see below
* is_polytope = is_polygon | is_polyhedron
* is_polygon -- The shape is known to be a polygon due to its construction,
  and that construction provides a vertex list, which can be queried.
  Some 2D shapes may be polygons, but TeaCAD doesn't know it, and doesn't
  have a vertex list--for those shapes, is_polygon is false.
* is_polyhedron -- has vertices and faces, which can be queried.
* is_convex -- The shape is known to be convex based on how it was constructed.
  If is_convex is false, then it still might be convex, but a more sophisticated
  (ie, time consuming) analysis is required.
* is_finite -- same comment as is_convex.

other query operations:
* bbox(shape)
* for polygons:
  * 2d_points(p), 2d_paths(p) provide arguments to polygon().
* for polyhedra:
  * 3d_points(p), 3d_faces(p) provide arguments to polyhedron().

## Primitive Shapes
* Includes the OpenSCAD primitive shapes, but polyhedral and curved shapes
  are distinct: eg, cylinder() vs prism().
* Includes the Conway operator seed shapes.
* SGDL has one primitive shape, the quadric. Include it?
* Plus other stuff.

So,
* cube(s) or cube[x,y,z]
* sphere(d) or sphere[xd,yd,zd]
* cylinder(h,d)
* cone(h,d)
* prism(h,d)
* antiprism(h,d)
* pyramid(n,h,d)
* tetrahedron, hexahedron, octahedron, dodecahedron, icosahedron
* geode

## Parameters for Primitive Shapes
* Polyhedral and curved shapes are distinct: eg, cylinder() vs prism().
* There's no overloading of multiple geometric primitives into a single module,
  eg, cylinder() vs cone().
* There are no alternative choices of keyword arguments in a module,
  such as r= vs d= in cylinder. All circles are specified by diameters,
  not radii, for consistency with cube().

This splits up cylinder into multiple primitives with simpler argument lists.
* cylinder(h, d)
* cone(h, d, d2=0)
* prism(h, n, od|id|el=)
* pyramid(h, n, od|id|el=)

The size of a regular polygon is specified in one of three ways:
* `od` is the outside diameter: the diameter of a circumscribed circle that
  passes through each vertex.
* `id` is the inside diameter: the diameter of an inscribed circle that passes
  through the centre of each edge.
* `el` is the edge length.

Suppose you are 3D printing an engineered shape where parts need to fit
together. You need a circular hole just wide enough for a cylindrical rod to
friction fit. You'd like to specify the inradius, and require that whatever
polygonal approximation to a circle that is eventually used by the STL file
will have this inradius.

In another example, you are 3D printing a cylinder that must friction fit the
above hole, and you'd like to specify the circumradius.

In a third example, you are 3D printing a cylinder, and you want to specify
the "average" radius, with an error tolerance, the amount by which the polygon
can be more or less than the specified radius.

What data structure can preserve this information until tesselation occurs?
A CSG tree.

## What is a Shape?
Obviously, cube() returns a 3DShape, square() returns a 2DShape,
and both are classified as Shape.

What about objects (aka OpenSCAD groups)?
* In TeaCAD, I want to generalize objects so that the list component can
  contain any kind of value. But I want to support OpenSCAD group semantics.
  So an object is a shape if it is empty, or if all of its list elements
  are shapes.
* If you apply shape predicates or unary shape operations to a group of shapes,
  then it is implicitly treated as a union.

An object may contain a mix of 2D and 3D shapes, as with OpenSCAD groups.
* You can't union them and then export the result as STL. OpenSCAD 'render'
  gives an error. But OpenSCAD union with preview doesn't give an error.
* A reasonable approach: CSG operations on mixed objects give an error.
  The 2DShape and 3DShape predicates are false on a mixed object (ie, not an
  error). Some other predicates could tolerate mixed objects,
  eg Finite could be true if each element is Finite, ditto for Polytope.
* RapCAD is apparently more tolerant of a mix of 2D and 3D shapes.
  Apparently CGAL supports operations such as the intersection of a 3D shape
  and a 2D shape. The RapCAD approach suits the TeaCAD design goals better.

PLASM supports arbitrary-dimensional shapes. Which is useful for doing math art.
Eg, forming the 3D projection of 4D objects. (Ooh, MathArt as a project name?)
CGAL has a d-dimensional geometry kernel, but it (apparently?) doesn't do much.
The "Boolean" set operations are supported on 2D and 3D Nef polytopes only.

## Polytope representation.
In the general case, a polygon or polyhedron shape can have
multiple disjoint figures, each with zero or more interior voids.
* The interface for querying the vertices and faces of a polytope
  should take this into account.
* The polygon() and polyhedron() constructors should support the general case.
* The output of the query functions should be accepted as input to the
  constructor functions.

A "general polyhedron" is defined to be the result of taking convex polyhedra
and combining them using arbitrary CSG operations. This can result in:
 1. 2 or more disjoint volumes
 2. a volume containing one or more internal voids
 3. two distinct, non-overlapping volumes that share a vertex or edge
    (not 2-manifold)
 4. a single non-convex volume with two arms that touch each other
    at a vertex or edge (not 2-manifold)

There are several different B-Rep representations of polyhedra:
* A Nef Polyhedron representation, which is a collection of half-planes
  combined with set operations (complement, intersection, difference).
  This is fully general.
* An STL file, which is a list of triangles, and no additional information.
  A 2-manifold volume, or collection of disjoint volumes, can be reconstructed
  from the triangle list. But this representation is not fully general,
  because cases 3 & 4 can't be unambiguously represented.
  As noted in the introduction of:
  http://www.ics.uci.edu/~dock/manuals/cgal_manual/Nef_3/Chapter_main.html
* An AMF or 3MF file, which is slightly different from STL.
  There is a list of vertices, and there is a list of triangles, where the
  triangle vertices are given as indexes into the vertex list.
  It is possible to store duplicate copies of vertices in the vertex list,
  so that these copies will have different index numbers, so that we can
  avoid a non-manifold mesh in the triangle list. This allows us to handle
  cases 3 and 4. More information about how this representation is
  interpreted can be found in section 4.1 of:
  http://3mf.io/wp-content/uploads/2015/04/3MFcoreSpec_1.0.1.pdf

Based on this analysis, the OpenSCAD "polyhedron" primitive is capable
of representing a general polyhedron.

CGAL supports Nef Polyhedra, so this is another possible representation.
