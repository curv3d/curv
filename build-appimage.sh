#!/bin/bash

set -x
set -e

# do cleanup, even if errors occur
cleanup ()
{
  # remove temp downloaded files and AppImage-build artifacts
  rm -f linuxdeploy-x86_64.AppImage
  rm -f appimagetool-x86_64.AppImage
  rm -f AppRun-x86_64
  rm -f $desktop_filepath
  rm -f $icon_filepath
}
trap cleanup EXIT

# variables
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_DIR=release
APPDIR=$BUILD_DIR/AppDir
ID=org.curv3d.curv # software id

cd "$REPO_ROOT"

# remove AppDir from previous builds
rm -rf $APPDIR

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

# create AppImage's icons using curv
icon_filepath=$BUILD_DIR/curv.png
./$APPDIR/usr/bin/curv -o $icon_filepath -O xsize=512 -O ysize=512 icon.curv

# kill dummy X server
if [[ ! -z $dummy_xconf ]]; then
  sudo kill $(pgrep X)
  rm $dummy_xconf
fi

# create desktop entry for AppImage
desktop_filepath=$BUILD_DIR/$ID.desktop
cat > $desktop_filepath << EOL
[Desktop Entry]
Name=Curv
GenericName=2D/3D Graphics and Model Editor
Exec=curv %U
Type=Application
StartupNotify=true
Icon=curv
StartupWMClass=curv
Categories=Graphics
Terminal=true
Comment=Art creator app using mathematics
EOL

# add metainfo to AppDir
mkdir -p $APPDIR/usr/share/metainfo
cp metainfo.xml $APPDIR/usr/share/metainfo/$ID.appdata.xml

# download precompiled AppRun binary from official repo
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64
chmod +x AppRun-x86_64

# now, build AppImage using linuxdeploy
# download linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# bundle shared libraries for curv to AppDir and build AppImage from AppDir
export UPDATE_INFORMATION="gh-releases-zsync|curv3d|curv|latest|Curv-*x86_64.AppImage.zsync"
./linuxdeploy-x86_64.AppImage \
  --appdir=$APPDIR \
  --custom-apprun=AppRun-x86_64 \
  --desktop-file=$desktop_filepath \
  --icon-file=$icon_filepath \
  --output=appimage

# move built AppImage into release folder
mv Curv-*.AppImage $BUILD_DIR
mv Curv-*.AppImage.zsync $BUILD_DIR