#!/bin/bash

set -x
set -e

# building in temporary directory to keep system clean
# use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" appimage-build-XXXXXX)

# make sure to clean up build dir, even if errors occur
cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

# store repo root as variable
REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
OLD_CWD=$(readlink -f .)

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with CMake
# we need to explicitly set the install prefix, as CMake's default is /usr/local for some reason...
cmake "$REPO_ROOT" -DCMAKE_INSTALL_PREFIX=/usr

# build project and install files into AppDir
make -j$(nproc)
make install DESTDIR=AppDir

# now, build AppImage using linuxdeploy and appimagetool
# download linuxdeploy and appimagetool
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage

# make them executable
chmod +x linuxdeploy*.AppImage
chmod +x appimagetool-x86_64.AppImage

# create desktop entry
cat > curv.desktop << EOL
[Desktop Entry]
Name=curv
GenericName=
Exec=./AppRun %U
Type=Application
StartupNotify=true
Path=
Icon=curv
StartupWMClass=curv
Categories=Graphics
Terminal=true
Comment=Art creator app using mathematics
EOL

# patch absolute paths (make binary relocatable)
sed -i -e 's#/usr#././#g' AppDir/usr/bin/curv

# copy AppRun
apprun_filename="$OLD_CWD/AppRun"
chmod +x "$apprun_filename"
cp "$apprun_filename" AppRun

# configure the dummy X server
sudo apt update
sudo apt install xserver-xorg-video-dummy -y
cat > dummy-1920x1080.conf << EOL
Section "Monitor"
  Identifier "Monitor0"
  HorizSync 28.0-80.0
  VertRefresh 48.0-75.0
  # https://arachnoid.com/modelines/
  # 1920x1080 @ 60.00 Hz (GTF) hsync: 67.08 kHz; pclk: 172.80 MHz
  Modeline "1920x1080_60.00" 172.80 1920 2040 2248 2576 1080 1081 1084 1118 -HSync +Vsync
EndSection
Section "Device"
  Identifier "Card0"
  Driver "dummy"
  VideoRam 256000
EndSection
Section "Screen"
  DefaultDepth 24
  Identifier "Screen0"
  Device "Card0"
  Monitor "Monitor0"
  SubSection "Display"
    Depth 24
    Modes "1920x1080_60.00"
  EndSubSection
EndSection
EOL
sudo X -config dummy-1920x1080.conf
export DISPLAY=:0

# create icon
icon_filename="curv.png"
./AppDir/usr/bin/curv -o "$icon_filename" -O xsize=512 -O ysize=512 "$OLD_CWD/icon.curv"

# bundle shared libraries for Curv to AppDir, and build AppImage
./linuxdeploy-x86_64.AppImage --appdir AppDir -d curv.desktop -i "$icon_filename" --custom-apprun AppRun
./appimagetool-x86_64.AppImage AppDir

# move built AppImage back into original CWD
mv curv*.AppImage "$OLD_CWD"