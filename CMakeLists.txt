project(curv)
cmake_minimum_required(VERSION 3.5)
include_directories(.)

add_executable(tcad eval.cc exception.cc tcad.cc parser.cc readlinex.c scanner.cc token.cc)
target_link_libraries(tcad PUBLIC readline)

add_executable(tests test/tests.cc test/value.cc)
target_link_libraries(tests PUBLIC gtest pthread)

#set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(TARGET tcad tests PROPERTY CXX_STANDARD 14)
