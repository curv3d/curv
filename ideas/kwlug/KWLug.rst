=================================================
Curv: A Language for making Art using Mathematics
=================================================

Outline of KWLUG talk. Should be about 30-40 minutes.

* What is Curv. Why you should be excited.
  * MIT Licence

* Background/Motivation
  * Marius's April 2014 KWLUG presentation on OpenSCAD
  * my enthusiasm, then desire for improvements
    * OpenSCAD2 project: first commit May 2015
    * Curv: first commit May 2016
  * a next generation redesign of OpenSCAD. Goals:
    * faster rendering
    * a better language:
      * function values
      * shape values
      * the ability to introspect shape values
      * record values
      * design by contract/good error messages
      * simple, orthogonal design
  * a better alternative to the polyhedron representation of 3D shapes
    * F-REP/Signed Distance Fields
    * GPU rendering

* The Core Language

** How to design a language that is both simple and powerful

"Entities must not be multiplied beyond necessity" -- Occam's Razor

Everything is a value, and there are only 7 types of value.
* Instead of integers and floats, I just have numbers.
* Instead of tuples, lists, vectors, and multidimensional arrays,
  I just have lists.
* Instead of modules, objects and classes, I just have records.
* Instead of parameterized types, parameterized modules, parameterized classes,
  and functions, I just have functions.
* Static type checking ruins this, so Curv is dynamically typed.

Object-oriented programming is too complicated, and not supported.
But the features found in Standard ML and Haskell for data abstraction,
such as algebraic data types, SML 'Structures' which encapsulate a data type
with its operations, Haskell type classes: these can be programmed in Curv
using record and function values, as idioms, without special language support.

* The Shape Library

It has:
* 2D and 3D primitive shapes
* all of the boolean operators
* transformations: rigid, non-rigid, 2D->3D, 3D->2D, repetition
* distance field operators: offset, shell, morph, blending
* emboss and engrave (?)

How to make smoothly curved organic forms.
* sphere, cylinder, cone, ellipsoid, torus
* offset, shell, morph, blending

Eg,
* offset a tetrahedron
* shell a cylinder

* Signed Distance Fields

* Live Demo - 30min mark
  * build up a 3D object by adding to a geometry pipeline
  * build up a complex colour function
