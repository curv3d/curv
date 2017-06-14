project(curv)
cmake_minimum_required(VERSION 3.5)
include_directories(. googletest/googletest/include)

add_subdirectory(googletest/googletest)

if (APPLE)
  # Homebrew installs a private copy of GNU readline, which we use, which
  # has a different API than the Apple version of readline.
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/opt/readline/include" )
  set( LibReadline /usr/local/opt/readline/lib/libreadline.dylib )
else ()
  set( LibReadline readline)
endif (APPLE)

FILE(GLOB CurvSrc "aux/*.c" "aux/*.cc" "curv/*.cc")
add_library(libcurv ${CurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB Src "cmd/*.cc")
add_executable(curv ${Src})
target_link_libraries(curv PUBLIC libcurv ${LibReadline} double-conversion boost_filesystem boost_system)

FILE(GLOB TestSrc "tests/*.cc")
add_executable(tester ${TestSrc})
if (!APPLE)
  # This is only supported on x86_64 Linux (gcc and clang).
  set( sanitize "-fsanitize=leak" )
endif ()
target_link_libraries(tester PUBLIC ${sanitize} gtest pthread libcurv double-conversion)

#set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(TARGET curv libcurv tester PROPERTY CXX_STANDARD 14)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester)

install(TARGETS curv RUNTIME DESTINATION bin)
install(FILES lib/std.curv DESTINATION lib)
