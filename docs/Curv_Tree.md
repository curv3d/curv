The Curv Compiler is an API for compiling "trees" into LLVM, GLSL or SPIR-V.
The input trees are supposed to be "syntax independent": they can represent
either tcad or scad programs.

It's not yet clear what the "trees" look like.
I should implement tcad first, and then decide how to separate the tcad
implementation into a "curv" layer and a "tcad" layer.
