=================================================
Curv: A Language for making Art using Mathematics
=================================================

Outline of KWLUG talk. Should be about 30-40 minutes.

Introduction
============
* What is Curv. Why you should be excited.
  * Open source on github with an MIT Licence

Background/Motivation
=====================
* Marius's April 2014 KWLUG presentation on OpenSCAD
* My experience with OpenSCAD:
  * It's brilliant.
    * procedurally generated, parametric 3D models.
      Named model parameters, conditionals, loops, user defined functions.
    * CSG and boolean operators. Pure functional style.
    * terse functional style and no boilerplate,
      compared to 3D modelling languages embedded in JavaScript, Python, etc.
    * nice GUI
  * It has limitations.
    * language is weak and complicated: 3 namespaces, functions vs modules,
      lists vs groups, can't pass functions as arguments, can't store
      shapes in variables. Can't query a shape, eg can't ask for bounding box.
    * curved surfaces & organic shapes are hard to achieve.
      * lots of triangles are needed to make curved surfaces look good,
        but this makes boolean operations (union etc) unusably slow.
      * few operations for creating curved surfaces.
* my enthusiasm, then desire for improvements
  * OpenSCAD2 project: first commit May 2015
  * Curv: first commit May 2016
* a next generation redesign of OpenSCAD. Goals:
  * richer, more powerful set of CSG primitives.
  * fast and accurate rendering of curved objects.
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

The Core Language
=================
Goals:
* Easy to use for novices and artists. Focus is artistic exploration,
  not software engineering.
* Powerful enough for experts. Libraries of new CSG operations can
  be written in Curv and distributed over the internet.
* Safe and secure. A Curv program downloaded over the internet can't
  encrypt all your files or exfiltrate personal information to a server.

Main features:
* Simple, elegant and powerful.
* Dynamically typed.
* Pure functional language. No file or network i/o. All functions are pure:
  the result depends only on the argument values. Programs are expressions.
* 7 data types: the 6 JSON types, plus function values.
* The language is a superset of JSON. Most JSON programs are valid Curv
  programs. Curv is a data interchange format for pure functional data.

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

The Shape Library
=================

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

Signed Distance Fields
======================

Live Demo - 30min mark
======================
  * build up a 3D object by adding to a geometry pipeline
  * build up a complex colour function
