project(curv)
cmake_minimum_required(VERSION 3.5)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR "Do not use 'cmake .'; instead, just type 'make'")
endif ()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include_directories(.
    extern/googletest/googletest/include
    extern/glslViewer/include
    extern/openvdb
    extern/blosc)

add_subdirectory(extern/googletest/googletest EXCLUDE_FROM_ALL)
add_subdirectory(extern/openvdb/openvdb EXCLUDE_FROM_ALL)
add_subdirectory(extern/glslViewer EXCLUDE_FROM_ALL)

if (APPLE)
  # Homebrew installs a private copy of GNU readline, which we use, which
  # has a different API than the Apple version of readline.
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/opt/readline/include" )
  set( LibReadline /usr/local/opt/readline/lib/libreadline.dylib )
  set( sanitize "" )
  FIND_LIBRARY(OPENGL_LIB OpenGL)
  set( CurvLibs ${OPENGL_LIB} )
else ()
  set( LibReadline readline)
  # The leak sanitizer is only available for 64 bit Linux (gcc and clang).
  # It adds minimal overhead, so we add it to all executables in debug builds.
  set( sanitize "-fsanitize=address" )
  set( CurvLibs GL )
endif (APPLE)

FILE(GLOB LibCurvSrc "libcurv/*.cc")
add_library(libcurv ${LibCurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB LibVgeomSrc "libvgeom/*.cc")
add_library(libvgeom ${LibVgeomSrc})
set_property(TARGET libvgeom PROPERTY OUTPUT_NAME vgeom)

FILE(GLOB Src "cmd/*.c" "cmd/*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC GlslViewer glfw ${CurvLibs} libcurv libvgeom ${LibReadline} double-conversion boost_filesystem boost_system openvdb_static Half tbb dl pthread)

FILE(GLOB TestSrc "tests/*.cc")
add_executable(tester EXCLUDE_FROM_ALL ${TestSrc})
target_link_libraries(tester PUBLIC gtest pthread libcurv double-conversion boost_filesystem boost_system)

set_property(TARGET curv libcurv libvgeom tester PROPERTY CXX_STANDARD 14)

set( gccflags "-Wall -Wno-unused-result" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${sanitize} -O1 -Werror" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester curv)

install(TARGETS curv RUNTIME DESTINATION bin)
install(FILES lib/std.curv DESTINATION lib)
