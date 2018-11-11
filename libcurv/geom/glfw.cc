// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/glfw.h>

namespace curv { namespace geom {

void glfw_set_context_parameters()
{
    // On MacOS, according to this document:
    //    https://www.khronos.org/opengl/wiki/OpenGL_Context
    // if you want a profile that supports shaders, you need to request
    // version 3.2+, and you need to specify forward compatibility.
    // This seems to be the minimum requirement for ImGui to run on MacOS.
    // Nothing greater than 3.2 is currently required by anything in Curv.
    // Also, Khronos says only request forward compat on MacOS.

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

// GLSL version 1.5 corresponds to OpenGL 3.2. This #version statement is
// required for ImGui to run on MacOS.
const char glsl_version[] = "#version 150";

bool opengl_init()
{
    return gladLoadGL() != 0;
}

}} // namespaces
