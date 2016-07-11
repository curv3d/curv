project(curv)
cmake_minimum_required(VERSION 3.5)
include_directories(.)

FILE(GLOB CurvSrc "aux/*.c" "aux/*.cc" "curv/*.cc")
add_library(libcurv ${CurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB Src "*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC libcurv readline double-conversion)

FILE(GLOB TestSrc "tests/*.cc")
add_executable(tests ${TestSrc})
target_link_libraries(tests PUBLIC gtest pthread libcurv double-conversion)

#set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(TARGET curv libcurv tests PROPERTY CXX_STANDARD 14)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror" )
