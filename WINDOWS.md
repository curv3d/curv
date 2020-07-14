# Installing and Running Curv on Windows 10
These instructions will install Curv on Windows 10 as a native 64 bit Windows
program.

This is accomplished by installing MSYS2, which is a Linux-like operating
environment. Then, using the MSYS2 interactive command line shell, Curv is
installed as an MSYS2 application. The MSYS2 installation will reside
in C:/msys64, which includes the entire Curv installation.

Once everything is installed, you run Curv by first running the MSYS2
shell from the Start menu:
```
    Start -> MSYS2 64bit -> MSYS2 MinGW 64bit
```
(There are 3 choices under "MSYS2 64bit", only one is correct.)
Now you can type the "curv" command in the terminal window, and follow
the usage instructions for Linux in the regular documentation.

## Building and Installing Curv

 1. On Windows, install [MSYS2 64bit](https://www.msys2.org/)
    Just follow the instructions from the web site.

 2. At the end of the installation, an "MSYS" window will open.
    The prompt says "MSYS", it does not say "MinGW64".
    We don't want to use this shell, so just close the window,
    or type "exit" and press ENTER.

 3. Open a "MinGW64" shell from the start menu:
    ```
    Start -> MSYS2 64bit -> MSYS2 MinGW 64bit
    ```

 4. Upgrade MSYS2.
    Within the MinGW64 shell, type:
    ```
    pacman -Syuu
    ```
    Reopen the shell if asked to do so.

 5. Install `git`:
    ```
    pacman -S --needed git
    ```
    Note: One time when I tried this, I got weird error messages due to
    a network problem. Running the command a second time succeeded.
 
 6. Use `git` to install the Curv source code:
    ```
    git clone https://github.com/curv3d/curv
    cd curv
    git submodule update --init
    ```
 
 7. Install build tools and libraries:
    ```
    pacman -S --needed diffutils mingw-w64-x86_64-clang make mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-mesa mingw-w64-x86_64-openexr mingw-w64-x86_64-intel-tbb mingw-w64-x86_64-glm mingw-w64-x86_64-glew mingw-w64-x86_64-dbus patch
    ```

 8. Download MINGW-packages:
    ```
    cd
    git clone https://github.com/msys2/MINGW-packages
    ```

 9. Build openvdb:
    ```
    cd MINGW-packages/mingw-w64-openvdb
    MINGW_INSTALLS=mingw64 makepkg-mingw -sLf
    ```

10. Install openvdb:
    ```
    pacman -U mingw-w64-x86_64-openvdb-*-any.pkg.tar.zst
    ```

11. Build curv:
    ```
    cd
    cd curv
    make
    ```

## Testing Curv

After using the build instructions, there is a Curv executable
in `release/curv.exe` under the `curv` directory.
Here are some commands to try:

    - Graphics card detection: `./release/curv.exe --version`

        This should mention your graphics card, if not, you only get software rendering without hardware acceleration.
    - Simple rendering: `./release/curv.exe ./examples/kaboom.curv`
    - Livemode with editor: `CURV_EDITOR=notepad ./release/curv.exe -le ./examples/liquid_paint.curv`
    - Fast 3D rendering with C++ compiler: `./release/curv.exe ./examples/mesh_only/klein.curv -o mesh.obj -O jit`

        This command assumes a C++ compiler available as `c++` on PATH. If you run it in the same MSYS environment as set up above, you already have `c++` available.
