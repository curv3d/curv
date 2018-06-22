project(libGlslViewer)
cmake_minimum_required(VERSION 3.5)

include_directories(include src ../..)

FILE(GLOB Src
    "src/*.cpp" "src/3d/*.cpp" "src/gl/*.cpp" "src/tools/*.cpp"
    "src/types/*.cpp"  "src/ui/*.cpp"
    "include/tinyobjloader/*.cc")
add_library(GlslViewer ${Src})
set_property(TARGET GlslViewer PROPERTY OUTPUT_NAME libGlslViewer)

set_property(TARGET GlslViewer PROPERTY CXX_STANDARD 14)

set( gccflags "-Wall -Wno-unused-result" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1 -Werror" )

if (APPLE)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_OSX" )
else ()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_LINUX" )
endif (APPLE)
