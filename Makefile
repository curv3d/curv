ifeq ($(shell uname -s|sed 's/MINGW.*/Windows/'),Windows)
	# By default, CMake chooses MSVC as the generator under Msys
	# Overwrite this behavior to choose Msys Makefiles
	cmake_args := -G"MSYS Makefiles"
else
	cmake_arsgs := ""
endif

release:
	rm -rf CMakeCache.txt CMakeFiles
	mkdir -p release
	cd release; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE)
install:
	mkdir -p release
	cd release; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE) install
upgrade:
	git pull origin master --no-rebase
	git submodule update --init
uninstall:
	mkdir -p release
	cd release; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Release ..
	cd release; $(MAKE) uninstall
debug:
	mkdir -p debug
	cd debug; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE)
test:
	mkdir -p debug
	cd debug; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tests
clean:
	rm -rf debug release libcurv/version.h
valgrind:
	mkdir -p debug
	cd debug; cmake $(cmake_args) -DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tester
	cd tests; valgrind ../debug/tester
valgrind-full:
	mkdir -p debug
	cd debug; cmake $(cmake_args)-DCMAKE_BUILD_TYPE=Debug ..
	cd debug; $(MAKE) tester
	cd tests; valgrind --leak-check=full ../debug/tester
.PHONY: release install upgrade uninstall test debug clean valgrind valgrind-full
