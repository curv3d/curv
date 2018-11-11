project(curv)
cmake_minimum_required(VERSION 3.5)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Do not use 'cmake .'; instead, just type 'make'")
endif ()

execute_process(COMMAND sh -c "${CMAKE_SOURCE_DIR}/cmake/make_version.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Global include directories, visible in subdirectories.
include_directories(.
    extern/googletest/googletest/include
    extern/glslViewer/include
    extern/glslViewer/src
    extern/openvdb
    extern/blosc
    extern/glfw/include
    extern/glad
    extern/imgui/examples/libs/gl3w)

if (APPLE)
    # Using Homebrew on macOS. Search /usr/local/include (bug #28):
    include_directories(SYSTEM /usr/local/include)
    link_directories(/usr/local/lib)
endif (APPLE)

add_subdirectory(extern/googletest/googletest EXCLUDE_FROM_ALL)
add_subdirectory(extern/openvdb/openvdb EXCLUDE_FROM_ALL)
add_subdirectory(extern/glslViewer EXCLUDE_FROM_ALL)
add_subdirectory(extern/glfw EXCLUDE_FROM_ALL)

# REPLxx library
file(GLOB ReplxxSrc "extern/replxx/src/*.cxx" "extern/replxx/src/*.cpp")
add_library(replxx ${ReplxxSrc})
target_include_directories(replxx
   PUBLIC extern/replxx/include
   PRIVATE extern/replxx/src)
set_property(TARGET replxx PROPERTY CXX_STANDARD 14)

# gl3w library, an OpenGL loader
add_library(gl3w
    extern/imgui/examples/libs/gl3w/GL/gl3w.c)
target_include_directories(gl3w
    PUBLIC extern/imgui/examples/libs/gl3w)
set_property(TARGET gl3w PROPERTY CXX_STANDARD 14)

# glad library, an OpenGL loader
add_library(glad
    extern/glad/glad/glad.c)
set_property(TARGET glad PROPERTY CXX_STANDARD 14)

# Dear IMGUI library
add_library(imgui
    extern/imgui/examples/imgui_impl_glfw.cpp
    extern/imgui/examples/imgui_impl_opengl3.cpp
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_widgets.cpp)
target_include_directories(imgui
    PUBLIC extern/imgui extern/imgui/examples)
target_link_libraries(imgui PUBLIC glfw gl3w)
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD )
set_property(TARGET imgui PROPERTY CXX_STANDARD 14)

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

# imdemo, will probably remove later
add_executable(imdemo
    extern/imgui/examples/example_glfw_opengl3/main.cpp)
target_include_directories(imdemo
    PUBLIC extern/imgui/examples/example_glfw_opengl3)
target_link_libraries(imdemo PUBLIC glfw glad imgui ${LibOpenGL})
set_property(TARGET imdemo PROPERTY CXX_STANDARD 14)

file(GLOB LibCurvSrc "libcurv/*.cc")
add_library(libcurv ${LibCurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB LibCurvGeomSrc "libcurv/geom/*.cc" "libcurv/geom/viewer/*.cc")
add_library(libcurv_geom ${LibCurvGeomSrc})
set_property(TARGET libcurv_geom PROPERTY OUTPUT_NAME curv_geom)
target_link_libraries(libcurv_geom PUBLIC imgui)

file(GLOB Src "curv/*.c" "curv/*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC libcurv libcurv_geom imgui GlslViewer glfw glad ${LibOpenGL} replxx double-conversion boost_iostreams boost_filesystem boost_system openvdb_static Half tbb dl pthread)

file(GLOB TestSrc "tests/*.cc")
add_executable(tester EXCLUDE_FROM_ALL ${TestSrc})
target_link_libraries(tester PUBLIC gtest pthread libcurv double-conversion boost_iostreams boost_filesystem boost_system)

set_property(TARGET curv libcurv libcurv_geom tester PROPERTY CXX_STANDARD 14)

set(gccflags "-Wall -Wno-unused-result" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${sanitize} -O1 -Werror" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester curv)

install(TARGETS curv RUNTIME DESTINATION bin)
install(FILES lib/std.curv DESTINATION lib)
install(DIRECTORY lib/curv DESTINATION lib)
install(FILES lib/curv.lang DESTINATION share/gtksourceview-3.0/language-specs)
