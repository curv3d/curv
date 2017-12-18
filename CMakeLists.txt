project(curv)
cmake_minimum_required(VERSION 3.5)
include_directories(. googletest/googletest/include)

add_subdirectory(googletest/googletest)

if (APPLE)
  # Homebrew installs a private copy of GNU readline, which we use, which
  # has a different API than the Apple version of readline.
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/opt/readline/include" )
  set( LibReadline /usr/local/opt/readline/lib/libreadline.dylib )
  set( sanitize "" )
else ()
  set( LibReadline readline)
  # The leak sanitizer is only available for 64 bit Linux (gcc and clang).
  # It adds minimal overhead, so we add it to all executables in debug builds.
  set( sanitize "-fsanitize=address" )
endif (APPLE)

FILE(GLOB CurvSrc "curv/*.cc")
add_library(libcurv ${CurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB Src "cmd/*.c" "cmd/*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC libcurv ${LibReadline} double-conversion boost_filesystem boost_system)

FILE(GLOB TestSrc "tests/*.cc")
add_executable(tester ${TestSrc})
target_link_libraries(tester PUBLIC gtest pthread libcurv double-conversion boost_filesystem boost_system)

set_property(TARGET curv libcurv tester PROPERTY CXX_STANDARD 14)

set( gccflags "-Wall -Werror -O1 -Wno-unused-result" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${sanitize}" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester curv)

install(TARGETS curv RUNTIME DESTINATION bin)
install(FILES lib/std.curv DESTINATION lib)
