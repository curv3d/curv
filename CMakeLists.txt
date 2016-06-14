project(curv)
cmake_minimum_required(VERSION 3.5)
include_directories(.)

FILE(GLOB CurvSrc "aux/*.c" "aux/*.cc" "curv/*.cc")
add_library(curv ${CurvSrc})

FILE(GLOB Src "*.cc")
add_executable(tcad ${Src})
target_link_libraries(tcad PUBLIC curv readline double-conversion)

FILE(GLOB TestSrc "test/*.cc")
add_executable(tests ${TestSrc})
target_link_libraries(tests PUBLIC gtest pthread curv double-conversion)

#set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(TARGET curv tcad tests PROPERTY CXX_STANDARD 14)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror" )
