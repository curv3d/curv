release: libcurv/version.h
	rm -rf CMakeCache.txt CMakeFiles
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; make
install: libcurv/version.h
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; make install
debug: libcurv/version.h
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make
test: libcurv/version.h
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tests
clean:
	rm -rf debug release
valgrind: libcurv/version.h
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tester
	cd tests; valgrind ../debug/tester
valgrind-full: libcurv/version.h
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tester
	cd tests; valgrind --leak-check=full ../debug/tester
libcurv/version.h:
	echo '#define CURV_VERSION "'`git describe --tags --always --dirty`'"' >,v
	if cmp -s ,v libcurv/version.h; then :; else cp ,v libcurv/version.h; fi
.PHONY: release install test debug clean valgrind valgrind-full libcurv/version.h
