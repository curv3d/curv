Curv is a virtual machine for 3D solid modelling.
It's a technology demo for the OpenSCAD community, and shows off some tech
that is proposed for incorporation into OpenSCAD.

The goal is to make OpenSCAD faster and more powerful:
* Make the core language itself more expressive by incorporating ideas from the
  OpenSCAD2 proposal: first class function values and "objects".
* Provide a more powerful and expressive set of geometric primitives.
* Add support for "functional representation":
  * Because it's a resolution independent representation for arbitrary curved
    shapes, and because it can represent huge of amounts detail (like high
    iteration fractals or digital fabrics) very cheaply.
  * Because it's so much easier to implement powerful new geometric primitives
    and operations using F-Rep than with B-Rep. Unlike ImplicitCAD, I want to
    provide an efficient low level F-Rep API in the OpenSCAD scripting language,
    so that you don't need to use the underlying implementation language (C++).
    For example, `sphere()` and `union()` are each 4 lines of code.
  * Because it can be executed quickly on a GPU, and will enable massive
    increases in rendering speed for certain types of models.

Curv will include a back end JIT compiler and code generator for OpenSCAD-like
languages. There are two targets: native machine code (via LLVM),
and GPU code (possibly targetting GLSL or SPIR-V). This back-end is syntax
independent, and can be used by multiple front ends implementing different
languages.

Curv will include a geometry engine that supports both boundary representation
and functional representation of colour 2D and 3D shapes. There is a C++ API.

TeaCAD is a language: it's a technology demo for what a next generation
OpenSCAD could look like. TeaCAD is a dynamically typed pure functional
language with 7 basic data types: Null, Boolean, Number, String, List,
Object and Function. (Geometric shapes are a kind of object.)

TeaCAD is inspired by the OpenSCAD2 proposal. It's not 100% backward compatible
with OpenSCAD, because that is a difficult problem. Instead, my strategy is to
offer a back end virtual machine and geometry engine that can be used by both
OpenSCAD and TeaCAD, so that a geometric model could be created using a mixture
of OpenSCAD and TeaCAD libraries. It's similar to how the Java VM enables
interoperability between JVM languages.

Using LLVM, the back end compiles OpenSCAD/TeaCAD code into C callable functions
and C++ callable classes, which call directly into the C/C++ runtime library.
This means we can build a bridge between OpenSCAD/TeaCAD and other languages,
in both directions.

The Curv project began in May 2016, and is a work in progress.
It will be some time before we are ready for a numbered release.
Contact the primary author Doug Moen via doug at the domain moens dot org.

== How to Build Curv and TeaCAD
After installing all of the dependencies, type `sh mk`.
So far, this has only been tested on Ubuntu 16.04.

== How to Run TeaCAD
The executable `build/tcad` is an interactive REPL loop.
You type a TeaCAD expression, it compiles and evaluates the expression
and prints the result.

== Dependencies
Here's how to install dependencies on Ubuntu 16.04.

The build system is `cmake`:
```
$ sudo apt-get install cmake
```

The C++ API documentation is generated using `doxygen`:
```
$ sudo apt-get install doxygen
```

The unit test framework is `googletest`:
```
$ sudo $SHELL
# apt-get install libgtest-dev
# cd /usr/src/gtest
# cmake
# make
# cp lib*.a /usr/lib
```

The GNU readline library:
```
$ sudo apt-get install libreadline-dev
```
