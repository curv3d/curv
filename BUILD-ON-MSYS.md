# Building Curv on Windows + MSYS2

1. Set-up of MSYS2 and Installation of required MSYS2 packages:

    1. On Windows install [MSYS2](https://www.msys2.org/)
    2. Within "MSYS2 MiNGW 64-bit" (accessible from start menu) run

        1. `pacman -Syuu` -- reopen the shell if asked to do so
        2. `pacman -S --needed diffutils mingw-w64-x86_64-clang make mingw-w64-x86_64-cmake git mingw-w64-x86_64-boost mingw-w64-x86_64-mesa mingw-w64-x86_64-openexr mingw-w64-x86_64-intel-tbb mingw-w64-x86_64-glm mingw-w64-x86_64-glew mingw-w64-x86_64-dbus patch doxygen`

           NB:

             - `diffutils` is required for binary `cmp`, which is somewhere used in the build process (in a Makefile)
             - `patch` is required for building the OpenVDB package below
             - `doxygen` is optional for generating API documentation
        3. Install the OpenVDB package for MSYS2

             1. Download the [`mingw64-openvdb` package source](https://github.com/msys2/MINGW-packages/tree/master/mingw-w64-openvdb) somewhere arbitrary

                  Downloading just the linked folder is sufficient. You might want to use your browser and copy/pasting than cloning -- the Git repo is *large*!
             2. Run `MINGW_INSTALLS=mingw64 makepkg-mingw -sLf` in that directory
             3. Run `pacman -U mingw-w64-x86_64-openvdb-7.0.0-1-any.pkg.tar.zst` in that directory (filename may differ slightly!)

2. Download curv sources:
    1. `git clone --recursive https://github.com/curv3d/curv`, `cd curv`
    2. `git submodule update --init`

3. Build curv and dependencies: `make`

     As a result, you get `./release/curv.exe` -- a **native** Windows executable. You should be a able to run this executable on every Windows system, even without MSYS, granted that you ship the required DLLs from the MSYS system, e.g. by putting them in the same directory.

3. Test

    - Graphics card detection: `./release/curv.exe --version`

        This should mention your graphics card, if not, you only get software rendering without hardware acceleration.
    - Simple rendering: `./release/curv.exe ./examples/kaboom.curv`
    - Livemode with editor: `CURV_EDITOR=notepad ./release/curv.exe -le ./examples/liquid_paint.curv`
    - Fast 3D rendering with C++ compiler: `./release/curv.exe ./examples/mesh_only/klein.curv -o mesh.obj -O jit`

        This command assumes a C++ compiler available as `c++` on PATH. If you run it in the same MSYS environment as set up above, you already have `c++` available.
