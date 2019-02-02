curvc is a simple program that contains the Curv translator, with few
other dependencies, and is linked as a static executable.
Usage:
    curvc filename
translates the Curv program, and outputs JSON-API to stdout.

This program is being used to implement a 'curv compile server'
for Sebastien's web GUI. It will likely be replaced by a WebAssembly module
in the future.

curvc is not part of the default build. To build it, use:
    make
    cd release
    make curvc

The build relies on static libraries. On Ubuntu, there seems to be no problem.
On Fedora, static libraries are not installed by default.
