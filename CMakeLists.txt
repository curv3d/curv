cmake_minimum_required(VERSION 3.12)

# Thanks https://stackoverflow.com/a/50313990
# Now how do we make this robust
if (APPLE)
    set(CMAKE_C_COMPILER "/usr/local/Cellar/llvm/15.0.7_1/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/local/Cellar/llvm/15.0.7_1/bin/clang++")
    set(OPENMP_LIBRARIES "/usr/local/Cellar/llvm/15.0.7_1/lib")
    set(OPENMP_INCLUDES "/usr/local/Cellar/llvm/15.0.7_1/include")
endif ()

project(curv)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Do not use 'cmake .'; instead, just type 'make'")
endif ()

# This is set to 1 on Lee's underpowered server.
# It disables the libfive and openvdb dependencies, which require
# lots of memory to compile.
set(LEAN_BUILD 0)
file(GLOB LeanSensitiveSrc curv/export_mesh.cc)
set_property(
    SOURCE ${LeanSensitiveSrc}
    APPEND PROPERTY COMPILE_DEFINITIONS
    LEAN_BUILD=${LEAN_BUILD})

execute_process(COMMAND sh -c "${CMAKE_SOURCE_DIR}/cmake/make_version.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Configure global include directories, visible in subdirectories.
include_directories(.
    extern/double-conversion
    extern/ordered-map/include
    extern/blosc
    extern/stb
    extern/glfw/include
    extern/glm
    extern/glad)
if (MSYS)
    # On MSYS we use MinGW packages for OpenVDB and GoogleTest
else ()
    include_directories(
	extern/googletest/googletest/include
	extern/openvdb/openvdb)
endif ()

if (APPLE)
    # Using Homebrew on macOS. Search /usr/local/include (bug #28) for Intel,
    # or /opt/homebrew for Apple Silicon (bug #128)
    # Also, we now depend on tbb@2020, not tbb, due to openvdb not supporting
    # latest version (June 5 2021).
    include_directories(SYSTEM /usr/local/include)
    include_directories(/usr/local/opt/tbb@2020/include)
    link_directories(/usr/local/opt/tbb@2020/lib)
endif (APPLE)

# Libraries
# ===============================================

# required for pkg_check_modules
find_package(PkgConfig REQUIRED)

# Eigen is used by libfive
pkg_check_modules(EIGEN REQUIRED eigen3>=3.2.92)

# libfive library
IF (NOT LEAN_BUILD)
    file(GLOB_RECURSE LibfiveSrc "extern/libfive/libfive/src/*.cpp")
    add_library(libfive ${LibfiveSrc})
    target_include_directories(libfive
        PUBLIC extern/libfive/libfive/include
        SYSTEM PUBLIC ${EIGEN_INCLUDE_DIRS})
    set_property(
        SOURCE extern/libfive/libfive/src/libfive.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS
        GIT_TAG="whatever"
        GIT_REV="whatever"
        GIT_BRANCH="whatever")
    set_property(TARGET libfive PROPERTY CXX_STANDARD 17)
endif()

# tmc library
add_library(tmc
    extern/dmc/DualMarchingCubes.cpp
    extern/dmc/UniformGrid.cpp)
set_property(TARGET tmc PROPERTY CXX_STANDARD 17)

# REPLxx library
file(GLOB ReplxxSrc "extern/replxx/src/*.cxx" "extern/replxx/src/*.cpp")
add_library(replxx ${ReplxxSrc})
target_include_directories(replxx
   PUBLIC extern/replxx/include
   PRIVATE extern/replxx/src)
set_property(TARGET replxx PROPERTY CXX_STANDARD 17)
target_compile_definitions(replxx PRIVATE REPLXX_STATIC=1)

# double-conversion library
file(GLOB DoubleConversionSrc "extern/double-conversion/double-conversion/*.cc")
add_library(double-conversion ${DoubleConversionSrc})
set_property(TARGET double-conversion PROPERTY CXX_STANDARD 17)

# glad library, an OpenGL loader
add_library(glad
    extern/glad/glad/glad.c)
set_property(TARGET glad PROPERTY CXX_STANDARD 17)

# Dear IMGUI library
add_library(imgui
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp)
target_include_directories(imgui
    PUBLIC extern/imgui extern/imgui/examples
    extern/imgui/backends)
target_link_libraries(imgui PUBLIC glfw glad)
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD )
set_property(TARGET imgui PROPERTY CXX_STANDARD 17)

# GL library (OpenGL or mesa or ...)
if (APPLE)
    set( sanitize "" )
    FIND_LIBRARY(OPENGL_LIB OpenGL)
    set( LibOpenGL ${OPENGL_LIB} )
elseif (MSYS)
    FIND_LIBRARY(OPENGL_LIB OpenGL NAMES "opengl32.dll.a" HINTS "/mingw64/lib")
    set( LibOpenGL ${OPENGL_LIB} )
else ()
    # The leak sanitizer is only available for 64 bit Linux (gcc and clang).
    # It adds minimal overhead, so we add it to all executables in debug builds.
    set( sanitize "-fsanitize=address" )
    set( LibOpenGL GL )
endif ()

# OpenVDB library
# NB: cannot use find_package(OpenVDB ...) due to https://github.com/AcademySoftwareFoundation/openvdb/issues/412
if (MSYS)
    set( LibOpenVDB "openvdb" )
    set( LibHalf "Half" )
else ()
    set( LibOpenVDB "openvdb_static" )
    set( LibHalf "" )
endif ()

# Boost library (Windows port required 1.72, 1.65 is latest for Ubuntu 18.04)
find_package(Boost 1.65 REQUIRED COMPONENTS iostreams system)

# Libraries End
# ===============================================


file(GLOB LibCurvSrc "libcurv/*.cc")
add_library(libcurv ${LibCurvSrc})
set_property(TARGET libcurv PROPERTY OUTPUT_NAME curv)

FILE(GLOB LibCurvIoSrc "libcurv/io/*.cc" "libcurv/viewer/*.cc")
add_library(libcurv_io ${LibCurvIoSrc})
set_property(TARGET libcurv_io PROPERTY OUTPUT_NAME curv_io)
target_link_libraries(libcurv_io PUBLIC imgui)

file(GLOB Src "curv/*.c" "curv/*.cc")
if (LEAN_BUILD)
    file(GLOB FatSrc "curv/libfive_mesher.cc" "curv/vdb_mesher.cc")
    list(REMOVE_ITEM Src ${FatSrc})
    message(lean source ${Src})
endif ()
add_executable(curv ${Src})
target_compile_definitions(curv PRIVATE REPLXX_STATIC=1)

if (MSYS)
    set(LibDl "")
else ()
    set(LibDl "dl")
endif ()
if (LEAN_BUILD)
    set(FatLibraries "")
else ()
    set(FatLibraries libfive ${LibOpenVDB} tbb ${LibHalf})
endif ()
set(Libs
    libcurv_io libcurv imgui glfw glad ${LibOpenGL} replxx double-conversion
    tmc ${FatLibraries}
    Boost::iostreams Boost::system
    pthread ${LibDl})

target_link_libraries(curv PUBLIC ${Libs})

file(GLOB TestSrc "tests/*.cc")
add_executable(tester EXCLUDE_FROM_ALL ${TestSrc})
target_link_libraries(tester PUBLIC gtest pthread libcurv libcurv_io double-conversion Boost::iostreams Boost::system)

set_property(TARGET curv libcurv libcurv_io tester PROPERTY CXX_STANDARD 17)

# Thanks https://stackoverflow.com/a/50313990, and no thanks Apple
if (APPLE)
    set(ompflags "")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(OpenMP_C "${CMAKE_C_COMPILER}")
        set(OpenMP_C_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
        set(OpenMP_C_LIB_NAMES "libomp" "libgomp" "libiomp5")
        set(OpenMP_libomp_LIBRARY ${OpenMP_C_LIB_NAMES})
        set(OpenMP_libgomp_LIBRARY ${OpenMP_C_LIB_NAMES})
        set(OpenMP_libiomp5_LIBRARY ${OpenMP_C_LIB_NAMES})
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
        set(OpenMP_CXX_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
        set(OpenMP_CXX_LIB_NAMES "libomp" "libgomp" "libiomp5")
        set(OpenMP_libomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
        set(OpenMP_libgomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
        set(OpenMP_libiomp5_LIBRARY ${OpenMP_CXX_LIB_NAMES})
    endif()
    if (OPENMP_FOUND)
        include_directories("${OPENMP_INCLUDES}")
        link_directories("${OPENMP_LIBRARIES}")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        # set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif(OPENMP_FOUND)
else ()
    set(ompflags ${OpenMP_C_FLAGS})
endif ()

set(gccflags "-Wall -Wno-unused-result ${ompflags}" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gccflags}" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${sanitize} -O1" )

add_custom_target(tests tester WORKING_DIRECTORY ../tests)
add_dependencies(tests tester curv)

install(TARGETS curv RUNTIME DESTINATION bin)
install(DIRECTORY lib/curv DESTINATION lib)
install(FILES lib/curv.lang DESTINATION share/gtksourceview-3.0/language-specs)

# Only generate this target if no higher-level project already has
if (NOT TARGET uninstall)
    add_custom_target(uninstall rm -rfv
        "${CMAKE_INSTALL_PREFIX}/bin/curv"
        "${CMAKE_INSTALL_PREFIX}/lib/curv"
        "${CMAKE_INSTALL_PREFIX}/lib/std.curv"
        "${CMAKE_INSTALL_PREFIX}/share/gtksourceview-3.0/language-specs/curv.lang"
    )
endif()

if (NOT DEFINED MSYS)
    # On MSYS we use MinGW packages for OpenVDB and GoogleTest.
    # Copy or stub definitions from extern/openvdb/CMakeLists.txt
    set(OpenVDB_MAJOR_VERSION 10)
    set(OpenVDB_MINOR_VERSION 0)
    set(OpenVDB_PATCH_VERSION 1)
    set(OpenVDB_ABI ${OpenVDB_MAJOR_VERSION})
    set(OPENVDB_ABI_VERSION_NUMBER ${OpenVDB_MAJOR_VERSION})
    set(OPENVDB_INSTALL_INCLUDEDIR openvdb_install_includedir)
    add_library(dummy cmake/dummy.cc)
    add_library(Blosc::blosc ALIAS dummy)
    add_library(TBB::tbb ALIAS dummy)
    if (NOT LEAN_BUILD)
        add_subdirectory(extern/openvdb/openvdb/openvdb EXCLUDE_FROM_ALL)
        set_property(TARGET openvdb_static PROPERTY CXX_STANDARD 17)
    endif()
    add_subdirectory(extern/googletest/googletest EXCLUDE_FROM_ALL)
endif ()

add_subdirectory(extern/glfw EXCLUDE_FROM_ALL)
