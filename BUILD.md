**Warning** to **git** users:
This project uses `git submodule`. Use:
* `git clone --recursive https://github.com/curv3d/curv` to clone the repo.
* `git submodule update --init` after pulling updates.

**Warning** to **cmake** users:
This project uses `make` as a wrapper around `cmake`.
Do not use `cmake .`: it won't work, and it will clobber the `Makefile`.
Instead, build and install `curv` using:
* `make`
* `make install`

# Installing 

## Ubuntu Linux 16.04 build instructions
* Open the Terminal application and run the following commands:
  * Install the relevant dependencies used to build `curv`:
  
  `sudo apt install cmake git-core libboost-all-dev libdouble-conversion-dev libopenexr-dev libtbb-dev libglm-dev gedit`
  
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
  * `brew install double-conversion`
  * `brew install gedit`
  * `brew install ilmbase openexr tbb`
  * `brew install glm`
  * `cd ~`
  * `git clone --recursive https://github.com/curv3d/curv`
  * `cd curv`
  * `make`
  * `make install`

## Upgrade to a new version of Curv
* Open the Terminal application and run the following commands:
  * `cd ~/curv`
  * `git pull origin master`
  * `git submodule update --init`
  * `make`
  * `make install` (or `sudo make install` on Linux)

## Uninstall Curv
* Open the Terminal application and run the following commands:
  * `cd ~/curv`
  * `make uninstall` (or `sudo make uninstall` on Linux)

# Testing
## A simple test

Run `curv -le myshape.curv` from a command line. It should open and editor window, type `cube` and then save the file. A window should open showing a cube that you can drag to interact with. The original console will give error messages and such.
