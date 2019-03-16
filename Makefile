release:
	rm -rf CMakeCache.txt CMakeFiles
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE)
install:
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE) install
uninstall:
	mkdir -p release
	cd release; cmake -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE) uninstall
debug:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE)
test:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tests
clean:
	rm -rf debug release libcurv/version.h
valgrind:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tester
	cd tests; valgrind ../debug/tester
valgrind-full:
	mkdir -p debug
	cd debug; cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tester
	cd tests; valgrind --leak-check=full ../debug/tester
.PHONY: release install test debug clean valgrind valgrind-full
