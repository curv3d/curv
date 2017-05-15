= The Curv Language Reference Manual

== Design Goals
The Curv language is two things:
* It is a human readable, text based file format for describing 2D and 3D
  geometric objects, and for sharing them over the internet.
* It is a very simple, dynamically typed, pure functional programming language.

Curv uses "functional representation" (F-Rep) to describe geometric objects.
This is a "volumetric" or solid-modeling approach.
* Unlike boundary representations which approximate a shape
  using polyhedra or B-Splines, the boundary of a Curv object is represented
  by an arbitrary mathematical function. There are no limits on the shapes
  that can be described. Polyhedral and B-Spline representations are
  subsets of the possible Curv representations, and can be dealt with as
  special cases.
* Curv is Turing complete. It's possible to describe shapes, like iterated
  fractals, which have a small, finite representation, but which are infinitely
  large, or which contain an infinite amount of detail.
* Like image and voxel representations, but unlike polyhedral meshes augumented
  with texture maps, Curv represents the colour of an object volumetrically.
  Each geometric point on the surface and interior of an object is assigned
  a colour using a colour function (F-Rep).
* Curv supports CSG (Constructive Solid Geometry), in which shapes can be
  specified by applying geometric operations (such as affine transformations
  and boolean set operations) to geometric primitives (such as
* Unlike image and voxel representations, but like vector representations
  (eg, SVG), Curv is resolution independent.
* Unlike vector representations such as SVG, Curv is extensible within the
  language itself, so that arbitrary new geometric primitives can be defined
  (any primitive that can be specified using mathematics). Curv files can
  refer to other Curv files over the internet using URLs, to reference
  libraries of shapes, primitives and operations created by other users.

Because Curv is a file format for sharing geometric objects over the internet,
it must be safe and secure. When you render a Curv object on your display,
the Curv program must not, as a side effect, modify local storage and install
malware, and it must not read local storage and exfiltrate confidential
information to the internet. This means that Curv cannot be a general purpose
programming language like Python. Instead, it is a pure functional language
in which there are no side effects, and in which there are further security
restrictions to prevent malicious code from being written.

If you want to do geometric programming using Curv within a general purpose
language like Python or Javascript, the solution is to use a binding of the
Curv API designed for your programming language.
