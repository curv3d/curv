# Installing and Running Curv on Windows 10
These instructions will install Curv on Windows 10 as a native 64 bit Windows
program.

This is accomplished by installing MSYS2, which is a Linux-like operating
environment. Then, using the MSYS2 interactive command line shell, Curv is
installed as an MSYS2 application. The MSYS2 installation will reside
in C:/msys64, and the Curv installation will reside in C:/msys64/curv.

Once everything is installed, you can use the "curv" command in a terminal
window (the Command Prompt, the PowerShell, or the MSYS2 shell window).
Then follow the usage instructions for Linux in the regular documentation.

To fully uninstall Curv:
 * Uninstall MSYS2 using the System Settings app.
 * Remove references to C:\msys64 from your PATH variable:
   see step 7 of the installation instructions.

## Building and Installing Curv

 1. On Windows, install [MSYS2 64bit](https://www.msys2.org/)
    Just follow the instructions from the web site.

 2. After the installation, an "MSYS" window will remain open.
    Close this window.
    (The prompt says "MSYS", it does not say "MinGW64".
    We don't want to use this type of shell.)

 3. Open a "MinGW64" shell window from the start menu:
    ```
    Start -> MSYS2 64bit -> MSYS2 MinGW 64bit
    ```

 4. Download the Curv install script. In the shell window, type:
    ```
    wget https://raw.githubusercontent.com/curv3d/curv/master/windows.sh
    ```

 5. Run the Curv install script:
    ```
    sh windows.sh
    ```
    This will: download additional MSYS2 packages using `pacman`,
    download the Curv source code using `git`, and build the Curv executable
    using `make`.
     * If the last line of output is `== SUCCESS ==`, then it worked.
     * If the last line of output is `== BUILD FAILED ==`, then open an
       issue in the github repo, or post a message to the Curv mailing list.
     * The script may fail due to a network problem. Fix your internet
       connection and try again. Running the script multiple times does
       not cause a problem: it will just pick up where it left off.

    The script may take a long time to run, so you might wish to enjoy
    lunch while it is running.

    The full pathname of the Curv executable is:
    `C:\msys64\curv\release\curv.exe`.

 6. To run Curv from the existing shell window, first type:
    ```
    source ~/.bashrc
    ```
    This forces the shell to re-read its startup file, thus making the
    new `curv` command available.

 7. In order to run Curv from the Command Prompt or the PowerShell,
    you need to add Curv to your PATH variable:
    * In the "Start search" box in the Windows 10 task bar,
      type "env" and hit ENTER.
    * Select "Edit environment variables for your account".
    * In the control panel window that pops up, click on "Path"
      then click the "Edit..." button.
    * In the "Edit environment variable" window that pops up,
      add two additional directories using the "New" button:
      * `C:\msys64\mingw64\bin`
      * `C:\msys64\curv\release`

## Running Curv
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
    cd c:/msys64/curv/examples
    curv kaboom.curv
    ```

  * Livemode with editor:
    ```
    cd c:/msys64/curv/examples
    curv -le liquid_paint.curv
    ```

  * Export 3D model to an OBJ file (polygon mesh) for 3D printing.
    ```
    curv -o klein.obj -O jit c:/msys64/curv/examples/mesh_only/klein.curv
    ```
    Note, this command uses the MSYS2 C++ compiler that you installed
    as part of the build instructions.
