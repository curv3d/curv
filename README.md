Curv is a language, virtual machine and geometry engine for 2D and 3D solid
modelling. It's inspired by OpenSCAD, as well as ImplicitCAD, Antimony, and
other open source solid modellers. The end goal is to contribute some
of the Curv technology back into the OpenSCAD project.

Curv is designed for advanced 3D printing. It supports complex models with
massive amounts of detail, and it supports the new breed of full colour (CMYKW)
and multi-material printers which can deposit a different colour/material
at every voxel.

The Curv geometry engine supports polyhedral meshes (for interoperability
with STL) and functional representation. F-Rep is important because:
* It's a resolution independent representation for arbitrary curved shapes,
  and can represent huge of amounts detail (like high iteration fractals
  or digital fabrics) very cheaply.
* It can be executed quickly on a GPU, and will enable massive increases
  in rendering speed for certain types of models.
* It's much easier to implement geometric primitives and operations
  using F-Rep than with B-Rep. For example, `sphere()` and `union()` are
  each 4 lines of code in the Curv language, and they are fast.

The Curv language is a simple, expressive, dynamically typed functional
language. The syntax is based on OpenSCAD, and is easy to use for beginners.
But it is optimized for computational geometry, and most of the geometric
primitives are implemented in the Curv language, not in C++.

The Curv language currently has a simple optimizing compiler that generates
trees, which are interpreted. As the project progresses, these trees will be
further compiled into other representations:
* For fast rendering:
  * The previewer compiles the language subset used for functional
    representations into GLSL GPU code.
  * The STL exporter may compile functional representations into native machine
    code using LLVM, or into OpenCL. TBD.
* Later, if script evaluation needs to be sped up, the options include:
  * A byte code compiler and interpreter. Benefits are ease of implementation,
    fast compilation and good debugging. We could expect the same performance
    as CPython.
  * A tracing JIT compiler, like Lua-JIT or Javascript. Very complex.
  * Use LLVM to compile to machine code. Very slow compilation. Moderate
    to high complexity depending on performance goals.
  * In a future web implementation, compilation to Javascript or WebAssembly?

Curv is designed for easy interoperability with other programming languages,
with special attention to C++, Javascript, OpenSCAD and Python.
* The 6 JSON data types are the core data types of the Curv language.
  That is important for compatibility with JSON, and maps well onto
  OpenSCAD, Javascript and Python.
* There is near term support for JSON import and export. A geometric model
  can be exported as a "CSG tree" using JSON syntax.
* Curv is a C++ library, with a stable API in version 1.0.
* The Curv runtime is written in C++, using reference counted shared pointers
  for memory management, and C++ exceptions for reporting errors. This allows
  native functions to be written in idiomatic C++. It avoids the complex API
  used by other VM runtimes for interacting with a tracing garbage collector.
* Curv 'modules' are the unit of interoperability with other languages.
  Modules written in Curv can be referenced from other languages,
  and vice versa. In the medium term, OpenSCAD scripts can be imported
  as Curv modules. In the long term, we will have two-way interoperability with
  C++, Python, and perhaps other languages. Compiling Curv modules to native
  code, shared objects or DLLs using LLVM may be part of this solution.
* Curv ought to run in a web browser, and Curv ought to be supported as
  a Javascript API. This might be achieved by compiling the Curv library and
  Curv modules into WebAssembly and/or Javascript.

Curv is a work in progress. The project is just beginning. See below for
a roadmap. Contributions are welcome.

## Origins of the project
Curv is an experimental sandbox for testing new ideas and technologies that
I'd like to see added to OpenSCAD.

Current goals:
* The language is simple, expressive and powerful. It incorporate ideas from
  the OpenSCAD2 proposal, especially functions as first class values.
* Better error reporting and debugging.
* Easy interoperability with other programming languages.
* The language implementation is fast.
* Provide a more powerful and expressive set of geometric primitives.
* Most geometric primitives are implemented in the language, not in C++.
* The representation of a shape is accessible from within the language.
* The geometry engine is fast.
* The system supports functional representation (F-Rep).

This project is the continuation of OpenSCAD2. That project stalled because
the backward compatibility scheme made it too complex to implement. More
generally, I can't see how to achieve the project goals by making incremental
changes to the OpenSCAD source. In order to achieve the performance goals,
I think we need a new architecture for the language evaluator, the previewer,
and the renderer. It's far too difficult to experiment with radical new designs
if backward compatibility needs to be maintained at every commit.

So, in order to make progress, I decided to build a new system from scratch,
with every line of code written with the new goals in mind, and ignoring
backward compatibility for now.

Once we have a working prototype, and benchmarks to demonstrate that the new
architecture can satisfy the performance goals, then it becomes reasonable
to consider how to bring some of the new code back into OpenSCAD.

## Road Map
The Curv project began in May 2016, and is a work in progress.
Contributions are welcome.
Contact the primary author Doug Moen via doug at the domain moens dot org.

**Release 0.1**
* The Curv core language, with 8 basic types:
  the data types Null, Boolean, Number, String, List and Record
  that are isomorphic to JSON, plus the code types Function and Module.
* A `curv` tool with an interactive REPL loop, and the ability to run scripts
  specified as command line arguments, similar to the `python` tool.
* A byte code compiler and interpreter, with performance roughly competitive
  with CPython.

**Release 0.2**
* A new primitive function, `shape3d`, for constructing shapes using F-Rep.
* A library of high level 3D primitives implemented in the Curv language itself,
  using functional representation. At least: `cube`, `sphere`,
  affine transformations, boolean CSG.
* A previewer that works by compiling F-Rep into GLSL, for execution on a GPU
  using ray marching.
* A geometric model can be exported as a "CSG tree" using JSON syntax.
  And also imported?

**Release 0.3**
* STL export. This might work by compiling the functional representation
  into native machine code using LLVM, then using Dual Contouring to generate
  a mesh. Maybe this can also be done on a GPU using OpenCL, but that seems
  more difficult.
* STL import?

## How to Build Curv
After installing all of the dependencies, type `sh mk`.
So far, this has only been tested on Ubuntu 16.04.

## How to Run Curv
The executable `build/curv` is an interactive REPL loop.
You type a Curv expression, it compiles and evaluates the expression
and prints the result.

## Dependencies
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
