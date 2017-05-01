# Converting a mesh to a signed distance field

OpenSCAD can import STL files.
Curv should support this as well.
One promising approach is to:
* Convert the STL file to a signed distance field (SDF), as an expensive offline
  operation.
* Import the signed distance field.

So we need an efficient representation of a pre-computed SDF.

Hierarchical hp-Adaptive Signed Distance Fields (2016)
Dan Koschier, Crispin Deul & Jan Bender
https://animation.rwth-aachen.de/media/papers/2016-SCA-HPSDF.pdf
* doesn't seem to be released as open source.

Polyharmonic radial basis functions are a promising representation of
distance fields for arbitrary shapes (conversion from scanned data & meshes).
* http://www3.cs.stonybrook.edu/~qin/courses/graphics/graphics-radial-basis-function.pdf
