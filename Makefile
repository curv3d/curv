release:
	rm -rf CMakeCache.txt CMakeFiles
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; make
install:
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; make install
debug:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make
test:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tests
clean:
	rm -rf debug release
valgrind:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tester
	cd tests; valgrind ../debug/tester
valgrind-full:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; make tester
	cd tests; valgrind --leak-check=full ../debug/tester
.PHONY: release install test debug clean valgrind valgrind-full
