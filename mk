cd $(dirname "$0")
mkdir -p build
cd build
# TODO: build a Release tree as well, somewhere else
cmake -DCMAKE_BUILD_TYPE=Debug ..
make "$@"
