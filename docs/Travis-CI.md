Use Travis-CI.org to build Curv for Mac, Linux (ubuntu trusty 14.04)
and Windows 64 (using MinGW or wclang github.com/tpoechtrager/wclang).

Note that wclang requires at least clang 3.7, and only 64 bit Windows is
fully supported for C++. Also, MinGW is a standard Ubuntu package, and is
more mature, so I should prefer that. Go with standard and best-of-breed
where possible.

Do I need to use clang for all of my builds?
* On Trusty (the most recent linux supported by Travis-CI),
  gcc doesn't fully support C++14, while clang does support C++14.
  Or use g++: `apt-get install c++-5`. Note that gcc might be faster than clang
  for direct threading using "labels as values": the issues are caching the IP
  (a label value) in a register, and the code generated for goto a label value.
  This was an issue in 2014, need to test with the latest clang to be sure.
* I'm using LLVM. Is there a compatibility issue with GCC?
  C++ name mangling in the C++11 library is incompatible, that's all I've got
  so far. Calls to C functions from jitted code should work.
