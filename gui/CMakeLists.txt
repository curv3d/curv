cmake_minimum_required(VERSION 2.8.11)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)
find_package(Qt5Gui)

# Tell CMake to create the helloworld executable
add_executable(gcurv main.cpp window.cpp)

target_link_libraries(gcurv Qt5::Core Qt5::Gui)
