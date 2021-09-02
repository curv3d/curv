#!/bin/bash

set -x
set -e

# do cleanup, even if errors occur
cleanup () {
  # remove temp downloaded files and AppImage-build artifacts
  rm -f linuxdeploy-x86_64.AppImage
  rm -f appimagetool-x86_64.AppImage
  rm -f $desktop_filename
  rm -f $icon_filename
}
trap cleanup EXIT

# store repo root as variable
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_DIR=release
APPDIR=$BUILD_DIR/AppDir

cd "$REPO_ROOT"

# build project and install files into AppDir/usr
make -j$(nproc)
make install DESTDIR=AppDir

# rename usr/local -> usr/
cd $APPDIR/usr/local && mv * .. && cd .. && rmdir local
cd "$REPO_ROOT"

# patch absolute paths (make binary relocatable)
sed -i -e 's#/usr#././#g' $APPDIR/usr/bin/curv

# configure the dummy X server
if [[ -z "$DISPLAY" ]]; then
  sudo apt update
  sudo apt install xserver-xorg-video-dummy -y

  dummy_xconf=dummy-1920x1080.conf
  cat > $dummy_xconf << EOL
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
  sudo X -config $dummy_xconf &
  export DISPLAY=:0
  sleep 1
fi

# create AppImage's icon using curv
icon_filename=$BUILD_DIR/curv.png
./$APPDIR/usr/bin/curv -o $icon_filename -O xsize=512 -O ysize=512 icon.curv

# kill dummy X server
if [[ ! -z $dummy_xconf ]]; then
  sudo kill $(pgrep X)
  rm $dummy_xconf
fi

# create desktop entry for AppImage
desktop_filename=$BUILD_DIR/curv.desktop
cat > $desktop_filename << EOL
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

# now, build AppImage using linuxdeploy and appimagetool
# download linuxdeploy and appimagetool
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage

# make them executable
chmod +x linuxdeploy-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage

# bundle shared libraries for curv to AppDir, and build AppImage from AppDir
./linuxdeploy-x86_64.AppImage \
  --appdir $APPDIR \
  --custom-apprun AppRun \
  -d $desktop_filename \
  -i $icon_filename
./appimagetool-x86_64.AppImage $APPDIR

# move built AppImage into release folder
mv curv*.AppImage $BUILD_DIR