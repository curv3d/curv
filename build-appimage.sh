#!/bin/bash

set -x
set -e

# do cleanup, even if errors occur
cleanup ()
{
  # remove temp downloaded files and AppImage-build artifacts
  rm -f linuxdeploy-x86_64.AppImage
  rm -f appimagetool-x86_64.AppImage
  rm -f $desktop_filepath
  rm -f $icon_filepath
}
trap cleanup EXIT

# utility function - rename directory
shopt -s dotglob
rnmdir ()
{
  local from=$1
  local to=$2

  if [[ -d "$from" ]]; then
    if [[ $(ls -A "$from") ]]; then
      for item in "$from"/*; do
        rnmdir "$item" "$to/$(basename "$item")"
      done
      [[ -d "$from" ]] && rm -rf "$from"
    else
      mkdir -p "$to"
      rmdir "$from"
    fi
  elif [[ -f "$from" ]]; then
    mkdir -p "$(dirname "$to")"
    mv -u "$from" "$to"
  fi
}

# variables
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_DIR=release
APPDIR=$BUILD_DIR/AppDir
HICOLOR_ICONS_PATH=$APPDIR/usr/share/icons/hicolor
ID=org.curv3d.curv # software id

cd "$REPO_ROOT"

# cleanup leftover artifacts in AppDir
rm -f $APPDIR/*.png
rm -f $APPDIR/*.svg
rm -f $APPDIR/*.desktop

# build project and install files into AppDir/usr
make -j$(nproc)
make install DESTDIR=AppDir

# rename usr/local -> usr/
rnmdir $APPDIR/usr/local $APPDIR/usr

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
icon_filename=curv.png
icon_filepath=$BUILD_DIR/$icon_filename
./$APPDIR/usr/bin/curv -o $BUILD_DIR/$icon_filename -O xsize=512 -O ysize=512 icon.curv
./$APPDIR/usr/bin/curv -o $HICOLOR_ICONS_PATH/256x256/apps/$icon_filename -O xsize=256 -O ysize=256 icon.curv
./$APPDIR/usr/bin/curv -o $HICOLOR_ICONS_PATH/128x128/apps/$icon_filename -O xsize=128 -O ysize=128 icon.curv
./$APPDIR/usr/bin/curv -o $HICOLOR_ICONS_PATH/64x64/apps/$icon_filename -O xsize=64 -O ysize=64 icon.curv

# kill dummy X server
if [[ ! -z $dummy_xconf ]]; then
  sudo kill $(pgrep X)
  rm $dummy_xconf
fi

# create desktop entry for AppImage
desktop_filepath=$BUILD_DIR/$ID.desktop
cat > $desktop_filepath << EOL
[Desktop Entry]
Name=curv
GenericName=
Exec=./AppRun %U
Type=Application
StartupNotify=true
Icon=curv
StartupWMClass=curv
Categories=Graphics
Terminal=true
Comment=Art creator app using mathematics
EOL

# add metainfo to AppDir
rm -rf $APPDIR/usr/share/metainfo
mkdir -p $APPDIR/usr/share/metainfo
cp metainfo.xml $APPDIR/usr/share/metainfo/$ID.appdata.xml

# now, build AppImage using linuxdeploy
# download linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# bundle shared libraries for curv to AppDir and build AppImage from AppDir
export UPDATE_INFORMATION="gh-releases-zsync|curv3d|curv|latest|Curv-*x86_64.AppImage.zsync"
./linuxdeploy-x86_64.AppImage \
  --appdir=$APPDIR \
  --custom-apprun=AppRun \
  --desktop-file=$desktop_filepath \
  --icon-file=$icon_filepath \
  --output=appimage

# move built AppImage into release folder
mv Curv-*.AppImage $BUILD_DIR
mv Curv-*.AppImage.zsync $BUILD_DIR