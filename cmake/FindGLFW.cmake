# find the GLFW headers and libraries
#
# GLFW_FOUND            set if GLFW is found.
# GLFW_INCLUDE_DIR      GLFW's include directory
# GLFW_glfw_LIBRARY     GLFW libraries

SET( GLFW_FOUND TRUE )
SET( GLFW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/glfw/include" )
SET( GLFW_glfw_LIBRARY glfw )
