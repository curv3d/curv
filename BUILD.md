## Ubuntu Linux 16.04 build instructions
* Install glslViewer (https://github.com/doug-moen/glslViewer)
* Open the Terminal application and run the following commands:
  * `sudo apt-get install cmake`
  * `sudo apt-get install libboost-all-dev libdouble-conversion-dev`
  * `sudo apt-get install libreadline-dev`
  * `sudo apt-get install libopenvdb-dev libopenexr-dev libtbb-dev`
  * `sudo apt-get install libglm-dev`
  * `cd ~`
  * `git clone https://github.com/doug-moen/curv`
  * `cd curv`
  * `make`
  * `sudo make install`

## macOS build instructions
* Install homebrew (http://brew.sh)
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
  * `git clone https://github.com/doug-moen/curv`
  * `cd curv`
  * `make`
  * `make install`
* Install glslViewer (https://github.com/doug-moen/glslViewer)

## Upgrade to a new version of Curv
* Open the Terminal application and run the following commands:
  * `cd ~/curv`
  * `git pull origin master`
  * `make`
  * `make install`
