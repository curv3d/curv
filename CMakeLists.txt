project(curv)
cmake_minimum_required(VERSION 3.5)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Do not use 'cmake .'; instead, just type 'make'")
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Global include directories, visible in subdirectories.
include_directories(.
    extern/googletest/googletest/include
    extern/glslViewer/include
    extern/glslViewer/src
    extern/openvdb
    extern/blosc
    extern/glfw/include)

if (APPLE)
    # Using Homebrew on macOS. Search /usr/local/include (bug #28):
    include_directories(SYSTEM /usr/local/include)
endif (APPLE)

add_subdirectory(extern/googletest/googletest EXCLUDE_FROM_ALL)
add_subdirectory(extern/openvdb/openvdb EXCLUDE_FROM_ALL)
add_subdirectory(extern/glslViewer EXCLUDE_FROM_ALL)
add_subdirectory(extern/glfw EXCLUDE_FROM_ALL)

# replxx library
file(GLOB ReplxxSrc "extern/replxx/src/*.cxx" "extern/replxx/src/*.cpp")
add_library(replxx ${ReplxxSrc})
target_include_directories(replxx
   PUBLIC extern/replxx/include
   PRIVATE extern/replxx/src)
set_property(TARGET replxx PROPERTY CXX_STANDARD 14)

if (APPLE)
    set( sanitize "" )
    FIND_LIBRARY(OPENGL_LIB OpenGL)
    set( LibOpenGL ${OPENGL_LIB} )
else ()
    # The leak sanitizer is only available for 64 bit Linux (gcc and clang).
    # It adds minimal overhead, so we add it to all executables in debug builds.
    set( sanitize "-fsanitize=address" )
    set( LibOpenGL GL )
endif (APPLE)

file(GLOB LibCurvSrc "libcurv/*.cc")
add_library(libcurv ${LibCurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB LibCurvGeomSrc "libcurv/geom/*.cc" "libcurv/geom/viewer/*.cc")
add_library(libcurv_geom ${LibCurvGeomSrc})
set_property(TARGET libcurv_geom PROPERTY OUTPUT_NAME curv_geom)

file(GLOB Src "cmd/*.c" "cmd/*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC libcurv libcurv_geom GlslViewer glfw ${LibOpenGL} replxx double-conversion boost_filesystem boost_system openvdb_static Half tbb dl pthread)

file(GLOB TestSrc "tests/*.cc")
add_executable(tester EXCLUDE_FROM_ALL ${TestSrc})
target_link_libraries(tester PUBLIC gtest pthread libcurv double-conversion boost_filesystem boost_system)

set_property(TARGET curv libcurv libcurv_geom tester PROPERTY CXX_STANDARD 14)

set(gccflags "-Wall -Wno-unused-result" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${sanitize} -O1 -Werror" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester curv)

install(TARGETS curv RUNTIME DESTINATION bin)
install(FILES lib/std.curv DESTINATION lib)

file(GLOB GeditDir
    "/usr/local/share/gtksourceview-[234].0/language-specs"
    "/usr/share/gtksourceview-[234].0/language-specs")
foreach(Dir ${GeditDir})
    install(FILES lib/curv.lang DESTINATION "${Dir}")
endforeach(Dir)
