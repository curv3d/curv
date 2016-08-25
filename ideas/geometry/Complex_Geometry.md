# Complex Geometry: Materials, Colours and Micro-fine Detail

This is a roadmap for evolving OpenSCAD to solve the following problems:
* Support objects with multiple materials and colours.
* Support complex objects with micro-fine detail (more structure than can
  be feasibly represented by a mesh).
* Better support for curved objects and organic shapes.

In support of complex objects, we'll add the following functionality:
* Functional geometry: the ability to define geometric objects
  using functions, instead of using a mesh. OpenSCAD will become a
  hybrid system, capable of supporting models using a mixture of these
  two representations (also called F-Rep and B-Rep, for Functional and
  Boundary Representation).
* Voxels: the ability to export a model as a voxel file (like SVX), instead of to
  a mesh file (like STL).

Functional geometry is also a better way to construct curved objects.

I will also consider what's needed to import and export models as
AMF, 3MF and SVX. All 3 formats support multiple materials and colours,
and SVX is a voxel file format, while the other 2 are mesh formats.

The reason for considering such a large set of changes all at once, is that
it helps us avoid designing ourselves into a corner. If we have a roadmap
for where we are going, we can add new features incrementally without fear
that we'll have to break backward compatibility later when the next problem needs
to be solved.



## Multiple Materials

## Multiple Colours
