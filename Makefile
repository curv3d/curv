tests:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build; make tests
install:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build; make install
curv:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build; make curv
clean:
	rm -rf build
valgrind:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build; make tester
	cd tests; valgrind ../build/tester
valgrind-full:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build; make tester
	cd tests; valgrind --leak-check=full ../build/tester
.PHONY: tests install curv clean valgrind valgrind-full
