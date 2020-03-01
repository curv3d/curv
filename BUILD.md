**Warning** to **git** users:
This project uses `git submodule`. Use:
* `git clone --recursive https://github.com/curv3d/curv` to clone the repo.
* `git submodule update --init` after pulling updates (or use `make update`)

**Warning** to **cmake** users:
This project uses `make` as a wrapper around `cmake`.
Do not use `cmake .`: it won't work, and it will clobber the `Makefile`.
Instead, build and install `curv` using:
* `make`
* `make install`

**C++ Notes**: Curv requires a C++ 14 compiler.
You can use gcc 5.4 (or later), or clang 3.4 (or later).

# Installing 

## Windows 10 using WSL
Note that 3D rendering will be slow, because WSL does not support GPU access.
All 3D rendering is done in software. See `issue #88`_ for details.
.. _`issue #88`: https://github.com/curv3d/curv/issues/88

Install WSL (Windows Services for Linux) + Ubuntu 18.04 LTS:
 * https://docs.microsoft.com/en-us/windows/wsl/install-win10

Launch the "Ubuntu 18.04 LTS" app. A terminal window will open.
 * sudo apt-get update
 * sudo apt install ubuntu-desktop mesa-utils

Then follow the Ubuntu 18.04 build instructions, installing *all* of the
mentioned packages, including "dev packages for your GPU".

At this point, Curv will run but you won't be able to open a graphics window.
For that you need an X11 window server. Install VcXsrv:
 * https://sourceforge.net/projects/vcxsrv/

Start VcXsrv using the XLaunch utility. Use these configuration settings:
 * multiple windows
 * display 0
 * start no client
 * disable native opengl

Back in the Ubuntu terminal window:
 * export DISPLAY=localhost:0
 * curv -x cube

For troubleshooting, look at `issue #88`_.

## Ubuntu Linux 18.04 build instructions
* Open the Terminal application and run the following commands:
  * Install the relevant dependencies used to build `curv`:
  
  `sudo apt install clang cmake git-core libboost-all-dev libopenexr-dev libtbb-dev libglm-dev gedit dbus-x11`
  
  * If you don't already have dev packages for your GPU installed -- eg running a **server version** -- then you'll probably need to also install these:
  
  `sudo apt-get install libxcursor-dev libxinerama-dev libxrandr-dev libglu1-mesa-dev libgles2-mesa-dev libgl1-mesa-dev libxi-dev`
  * now `cd` to your preferred installation directory below which the `curv` directory will be placed, eg `cd ~`
  
  * Now create your local copy of the git repo, change to it, make the program, package it and install that package:
  
  `git clone --recursive https://github.com/curv3d/curv; cd curv; make; sudo make install`
  
  These instructions have been tested on versions up to at least Ubuntu 18.04.1.
  
## macOS build instructions
* Install homebrew (http://brew.sh)
* Open the Terminal application and run the following commands:
  * `brew update`
  * `brew upgrade`
  * `brew install git`
  * `brew install cmake`
  * `brew install boost`
  * `brew install gedit`
  * `brew install ilmbase openexr tbb`
  * `brew install glm`
  * `cd ~`
  * `git clone --recursive https://github.com/curv3d/curv`
  * `cd curv`
  * `make`
  * `make install`

## Upgrade to a new version of Curv
Open the Terminal application and run the following commands:
 * `cd ~/curv`
 * `make upgrade`
 * `make`
 * `make install` (or `sudo make install` on Linux)

Note that `make upgrade` is equivalent to:
 * `git pull origin master`
 * `git submodule update --init`

## Uninstall Curv
Open the Terminal application and run the following commands:
 * `cd ~/curv`
 * `make uninstall` (or `sudo make uninstall` on Linux)

# Testing
Here are some simple tests to ensure that Curv is working correctly.
Open the Terminal application and type these commands.
The `$` represents your shell prompt, and the expected command output is shown.

1. Evaluate an arithmetic expression:
   ```
   $ curv -x 2+2
   4
   ```

2. Display a circle (2D graphics):
   ```
   $ curv -x circle
   2D shape 2×2
   ** a graphics window opens, displaying a yellow circle **
   ```
   If the graphics window could not be opened, an error message should be displayed.
   If you can't see the graphics window, make sure it is not behind another window.
   If there is a problem with the GPU driver, then the graphics window should still open,
   but it may turn black or display garbage.

3. Display a cube (3D graphics):
   ```
   $ curv -x cube
   3D shape 2×2×2
   ** a graphics window opens, displaying a yellow cube **
   ```

4. Run Curv in "live editing" mode:
   * Type the following command: `curv -le myshape.curv`
   * A text editor window will open (`gedit`). If you can't see this window, make sure it isn't behind another window.
   * Enter the word `cube` into the text editor. Save the file.
   * A graphics window should open displaying a yellow cube. You can rotate and zoom it using the mouse or trackpad.
   * If there is an error in your program, it will be displayed in the original terminal window.
