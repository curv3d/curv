// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "app.h"
#include "tools/text.h"
#include "tools/geom.h"
#include "gl/shader.h"
#include "gl/vbo.h"
#include "types/shapes.h"
#include <libcurv/geom/viewer/viewer.h>

namespace curv { namespace geom { namespace viewer {

// GLOBAL VARIABLES
//============================================================================
//

//  CAMERA
glm::mat3 u_view2d = glm::mat3(1.);
// These are the 'view3d' uniforms.
// Note: the up3d vector must be orthogonal to (eye3d - centre3d),
// or rotation doesn't work correctly.
glm::vec3 u_centre3d = glm::vec3(0.,0.,0.);
// The following initial value for 'eye3d' is derived by starting with [0,0,6],
// then rotating 30 degrees around the X and Y axes.
glm::vec3 u_eye3d = glm::vec3(2.598076,3.0,4.5);
// The initial value for up3d is derived by starting with [0,1,0], then
// applying the same rotations as above, so that up3d is orthogonal to eye3d.
glm::vec3 u_up3d = glm::vec3(-0.25,0.866025,-0.433013);

//  ASSETS
Vbo* vbo;

void Viewer::open()
{
    u_centre3d = glm::vec3(0.,0.,0.);
    u_eye3d = glm::vec3(2.598076,3.0,4.5);
    u_up3d = glm::vec3(-0.25,0.866025,-0.433013);

    bool headless = false;

    // Initialize openGL context
    initGL (window_pos_and_size_, headless);

    // Adding default deines
    defines_.push_back("GLSLVIEWER 1");

    // Start working on the GL context
    setup();
}

bool Viewer::draw_frame()
{
    if (glfwWindowShouldClose(window_))
        return false;

    // Update
    updateGL();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Something change??
    if (!next_frame())
        return false;

    // Draw
    draw();

    // Swap the buffers
    renderGL();

    return true;
}

void Viewer::close()
{
    //glfwGetWindowPos(window_,
    //    &viewer->window_pos_and_size_.x, &viewer->window_pos_and_size_.y);
    glfwGetWindowSize(window_,
        &window_pos_and_size_.w, &window_pos_and_size_.z);
    onExit();
}

void Viewer::setup()
{
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    //  Load Geometry
    //
    vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    //  Build shader;
    //
    vertSource_ = vbo->getVertexLayout()->getDefaultVertShader();
    shader_.load(fragsrc_, vertSource_, defines_, verbose_);

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Viewer::draw()
{
    shader_.use();

    // Pass uniforms
    shader_.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    if (shader_.needTime()) {
        shader_.setUniform("u_time", float(getTime()));
    }
    if (shader_.needDelta()) {
        shader_.setUniform("u_delta", float(getDelta()));
    }
    if (shader_.needMouse()) {
        shader_.setUniform("u_mouse", getMouseX(), getMouseY());
    }
    if (shader_.needView2d()) {
        shader_.setUniform("u_view2d", u_view2d);
    }
    if (shader_.needView3d()) {
        shader_.setUniform("u_eye3d", u_eye3d);
        shader_.setUniform("u_centre3d", u_centre3d);
        shader_.setUniform("u_up3d", u_up3d);
    }

    glm::mat4 mvp = glm::mat4(1.);
    shader_.setUniform("u_modelViewProjectionMatrix", mvp);

    vbo->draw(&shader_);
}

void Viewer::onKeyPress(int _key)
{
    if (_key == 'q' || _key == 'Q') {
        glfwSetWindowShouldClose(window_, GL_TRUE);
    }
}

void onMouseClick(float _x, float _y, int _button)
{
}

void onScroll(float _yoffset)
{
    // Vertical scroll button zooms u_view2d and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        u_view2d = glm::translate(u_view2d, origin);
        u_view2d = glm::scale(u_view2d, zoom);
        u_view2d = glm::translate(u_view2d, -origin);

        // zoom view3d
        u_eye3d = u_centre3d + (u_eye3d - u_centre3d)*z;
    }
}

void onMouseDrag(float _x, float _y, int _button)
{
    if (_button == 1){
        // Left-button drag is used to pan u_view2d.
        u_view2d = glm::translate(u_view2d, -getMouseVelocity());

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        u_eye3d -= u_centre3d;
        u_up3d -= u_centre3d;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (getMouseVelX() / getWindowWidth()) * tau;
        u_eye3d = glm::rotate(u_eye3d, -xangle, u_up3d);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (getMouseVelY() / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        u_eye3d = glm::rotate(u_eye3d, -yangle, haxis);
        u_up3d = glm::rotate(u_up3d, -yangle, haxis);
        //
        u_eye3d += u_centre3d;
        u_up3d += u_centre3d;
    } else {
        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(u_eye3d - u_centre3d);
        glm::vec3 voff = glm::normalize(u_up3d)
            * (getMouseVelY()/getWindowHeight()) * dist3d;
        u_centre3d -= voff;
        u_eye3d -= voff;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        glm::vec3 hoff = glm::normalize(haxis)
            * (getMouseVelX()/getWindowWidth()) * dist3d;
        u_centre3d += hoff;
        u_eye3d += hoff;
    }
}

void onViewportResize(int _newWidth, int _newHeight)
{
}

void Viewer::onExit()
{
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // DELETE RESOURCES
    delete vbo;
}

}}}
