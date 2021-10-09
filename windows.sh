for pkg in $(cat <<END
    git
    make
    diffutils
    mingw-w64-x86_64-clang
    mingw-w64-x86_64-cmake
    mingw-w64-x86_64-boost
    mingw-w64-x86_64-mesa
    mingw-w64-x86_64-openexr
    mingw-w64-x86_64-intel-tbb
    mingw-w64-x86_64-glm
    mingw-w64-x86_64-glew
    mingw-w64-x86_64-dbus
    mingw-w64-x86_64-openvdb
    mingw-w64-x86_64-gtest
    mingw-w64-x86_64-libpng
    mingw-w64-x86_64-eigen3
    winpty
END
)
do
    echo "== installing '$pkg'"
    pacman -S --needed --noconfirm $pkg || exit 1
done

if test ! -d /curv; then
    cd /
    echo == installing Curv source code
    git clone https://github.com/curv3d/curv || {
        echo git clone failed
        rm -r curv
        exit 1
    }
fi

cd /curv || exit 1
echo == upgrading Curv source code
make upgrade || exit 1

echo == building Curv executable
make || {
    echo == BUILD ERROR ==
    exit 1
}

grep -q 'alias curv=' ~/.bashrc || {
    echo "== adding 'curv' command to MinGW64 shell"
    echo 'alias curv="winpty /curv/release/curv"' >> ~/.bashrc
    source ~/.bashrc
}
echo == SUCCESS ==
