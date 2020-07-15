# Installing and Running Curv on Windows 10
These instructions will install Curv on Windows 10 as a native 64 bit Windows
program.

This is accomplished by installing MSYS2, which is a Linux-like operating
environment. Then, using the MSYS2 interactive command line shell, Curv is
installed as an MSYS2 application. The MSYS2 installation will reside
in C:/msys64, which includes the entire Curv installation.

Once everything is installed, you can type the "curv" command
in a standard terminal window, either the Command Prompt or the PowerShell.
Follow the usage instructions for Linux in the regular documentation.

If you uninstall MSYS2 using the System Settings app,
then you will remove both MSYS2 and Curv.

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
    This creates `release/curv.exe`.
    The full pathname of this executable is:
    `C:\msys64\home\User\curv\release\curv.exe`.

12. Add Curv to your PATH variable:
    * In the "Start search" box in the Windows 10 task bar,
      type "env" and hit ENTER.
    * Select "Edit environment variables for your account".
    * In the control panel window that pops up, click on "Path"
      then click the "Edit..." button.
    * In the "Edit environment variable" window that pops up,
      add two additional directories using the "New" button:
      * `C:\msys64\mingw64\bin`
      * `C:\msys64\home\User\curv\release`

## Running Curv
If you try to invoke Curv from an MSYS2 terminal window,
it will _mostly_ work, but the interactive REPL (which you get by typing
the "curv" command with no arguments) will not work correctly.
This is caused by a bug in the MSYS2 terminal emulator.

Therefore, it is recommended that you run Curv from a standard Windows
terminal window, either the Command Prompt, or the PowerShell.

Here are some commands to try:

  * Print version information:
    ```
    curv --version
    ```
    This version information should be included in any bug report.
    The command works even when a lot of other things are broken.
    It will also describe your graphics card, which is critical information
    if the graphics are broken or slow.

  * Render some 3D graphics:
    ```
    cd C:\msys64\home\User\curv\examples
    curv kaboom.curv
    ```

  * Livemode with editor:
    ```
    cd C:\msys64\home\User\curv\examples
    curv -le liquid_paint.curv
    ```

  * Export 3D model to an OBJ file (polygon mesh) for 3D printing.
    ```
    curv -o klein.obj -O jit C:\msys64\home\User\curv\examples\mesh_only\klein.curv
    ```
    Note, this command uses the MSYS2 C++ compiler that you installed
    as part of the build instructions.
