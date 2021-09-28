// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_GLFW_H
#define LIBCURV_GEOM_GLFW_H

// A platform-independent header for OpenGL programming.
// * We use glad/glad.h as a platform-independent header file for OpenGL.
//   * Tried asking GLFW to include <GL/glext.h>, which worked fine with
//     glslViewer on Linux and MacOS. There was a problem with ImGUI on MacOS.
//   * Tried gl3w, which is the default loader for ImGui, but there was an
//     problem with glslViewer on MacOS.
// * We use GLFW as a platform-independent API for creating windows,
//   creating OpenGL contexts, reading keyboard and mouse input.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace curv { namespace geom {

// Set parameters for the OpenGL context version and profile, before creating
// a window in glfw. Curv currently requires OpenGL 3.2, since that seems to
// be the minimum version for ImGui to work on MacOS. For simplicity, all
// Curv code is locked to a single OpenGL version on all platforms.
void glfw_set_context_parameters();

// This is the string "#version 150", which corresponds to OpenGL 3.2.
extern const char glsl_version[];

// Dynamically load the OpenGL library.
// You must create an OpenGL context first.
// Returns true if the load succeeds.
bool opengl_init();

}} // namespaces
#endif // header guard
