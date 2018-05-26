**Warning** to **git** users:
This project uses `git submodule`. Use:
* `git clone --recursive https://github.com/doug-moen/curv` to clone the repo.
* `git submodule update --init` after pulling updates.

**Warning** to **cmake** users:
This project uses `make` as a wrapper around `cmake`.
Do not use `cmake .`: it won't work, and it will clobber the `Makefile`.
Instead, build and install `curv` using:
* `make`
* `sudo make install`

## Ubuntu Linux 16.04 build instructions
* Install glslViewer (https://github.com/doug-moen/glslViewer)
* Open the Terminal application and run the following commands:
  * `sudo apt-get install cmake`
  * `sudo apt-get install libboost-all-dev libdouble-conversion-dev`
  * `sudo apt-get install libreadline-dev`
  * `sudo apt-get install libblosc-dev libopenexr-dev libtbb-dev`
  * `sudo apt-get install libglm-dev`
  * `cd ~`
  * `git clone --recursive https://github.com/doug-moen/curv`
  * `cd curv`
  * `make`
  * `sudo make install`

## macOS build instructions
* Install homebrew (http://brew.sh)
* Install glslViewer (https://github.com/doug-moen/glslViewer)
* Open the Terminal application and run the following commands:
  * `brew update`
  * `brew upgrade`
  * `brew install git`
  * `brew install cmake`
  * `brew install boost`
  * `brew install double-conversion`
  * `brew install readline`
  * `brew install gedit`
  * `brew install openvdb`
  * `brew install glm`
  * `cd ~`
  * `git clone --recursive https://github.com/doug-moen/curv`
  * `cd curv`
  * `make`
  * `make install`

## Upgrade to a new version of Curv
* Open the Terminal application and run the following commands:
  * `cd ~/curv`
  * `git pull origin master`
  * `git submodule update --init` after pulling updates.
  * `make`
  * `make install` (or `sudo make install` on Linux)
