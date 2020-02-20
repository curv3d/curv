// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/glfw.h>

namespace curv { namespace geom {

// Curv currently requires OpenGL 3.3.
// It's the most recent version of OpenGL that runs on my late 2010 Macbook Air.
// ImGui seems to require OpenGL 3.x to run on MacOS.
// I specifically need 3.3 for some planned (post v0.4) features.

void glfw_set_context_parameters()
{
    // On MacOS, according to this document:
    //    https://www.khronos.org/opengl/wiki/OpenGL_Context
    // if you want a profile that supports shaders, you need to request
    // version 3.2+, and you need to specify forward compatibility.
    // This seems to be the minimum requirement for ImGui to run on MacOS.

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

// This #version statement is required for ImGui to run on MacOS.
const char glsl_version[] = "#version 330";

bool opengl_init()
{
    return gladLoadGL() != 0;
}

}} // namespaces
