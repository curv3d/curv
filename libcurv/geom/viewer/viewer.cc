// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#include <libcurv/geom/viewer/viewer.h>

#include <iostream>
#include <sstream>

#include <libcurv/string.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "app.h"
#include <types/shapes.h>

namespace curv { namespace geom { namespace viewer {

void
Viewer::set_shape(Shape_Recognizer& shape)
{
    std::stringstream f;
    export_frag(shape, f);
    fragsrc_ = f.str();
}

void
Viewer::run()
{
    open();
    while (draw_frame());
    close();
}

bool Viewer::next_frame()
{
    return true;
}

void Viewer::set_frag(const std::string& fragSource)
{
    shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    shader_.load(fragSource, vertSource_, defines_, verbose_);
}

Viewer::Viewer()
{
    defines_.push_back("GLSLVIEWER 1");
}

void Viewer::open()
{
    u_centre3d_ = glm::vec3(0.,0.,0.);
    u_eye3d_ = glm::vec3(2.598076,3.0,4.5);
    u_up3d_ = glm::vec3(-0.25,0.866025,-0.433013);

    bool headless = false;

    // Initialize openGL context
    initGL (window_pos_and_size_, headless);

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
    vbo_ = rect(0.0,0.0,1.0,1.0).getVbo();

    //  Build shader;
    //
    vertSource_ = vbo_->getVertexLayout()->getDefaultVertShader();
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
        shader_.setUniform("u_mouse", mouse_.x, mouse_.y);
    }
    if (shader_.needView2d()) {
        shader_.setUniform("u_view2d", u_view2d_);
    }
    if (shader_.needView3d()) {
        shader_.setUniform("u_eye3d", u_eye3d_);
        shader_.setUniform("u_centre3d", u_centre3d_);
        shader_.setUniform("u_up3d", u_up3d_);
    }

    glm::mat4 mvp = glm::mat4(1.);
    shader_.setUniform("u_modelViewProjectionMatrix", mvp);

    vbo_->draw(&shader_);
}

void Viewer::onKeyPress(int _key)
{
    if (_key == 'q' || _key == 'Q') {
        glfwSetWindowShouldClose(window_, GL_TRUE);
    }
}

void Viewer::onScroll(float _yoffset)
{
    // Vertical scroll button zooms u_view2d_ and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        u_view2d_ = glm::translate(u_view2d_, origin);
        u_view2d_ = glm::scale(u_view2d_, zoom);
        u_view2d_ = glm::translate(u_view2d_, -origin);

        // zoom view3d
        u_eye3d_ = u_centre3d_ + (u_eye3d_ - u_centre3d_)*z;
    }
}

void Viewer::onMouseDrag(float _x, float _y, int _button)
{
    if (_button == 1){
        // Left-button drag is used to pan u_view2d_.
        u_view2d_ = glm::translate(u_view2d_, -mouse_.velocity);

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        u_eye3d_ -= u_centre3d_;
        u_up3d_ -= u_centre3d_;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (mouse_.velocity.x / getWindowWidth()) * tau;
        u_eye3d_ = glm::rotate(u_eye3d_, -xangle, u_up3d_);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (mouse_.velocity.y / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(u_eye3d_-u_centre3d_, u_up3d_);
        u_eye3d_ = glm::rotate(u_eye3d_, -yangle, haxis);
        u_up3d_ = glm::rotate(u_up3d_, -yangle, haxis);
        //
        u_eye3d_ += u_centre3d_;
        u_up3d_ += u_centre3d_;
    } else {
        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(u_eye3d_ - u_centre3d_);
        glm::vec3 voff = glm::normalize(u_up3d_)
            * (mouse_.velocity.y/getWindowHeight()) * dist3d;
        u_centre3d_ -= voff;
        u_eye3d_ -= voff;
        glm::vec3 haxis = glm::cross(u_eye3d_-u_centre3d_, u_up3d_);
        glm::vec3 hoff = glm::normalize(haxis)
            * (mouse_.velocity.x/getWindowWidth()) * dist3d;
        u_centre3d_ += hoff;
        u_eye3d_ += hoff;
    }
}

void Viewer::onExit()
{
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // DELETE RESOURCES
    delete vbo_;
}

}}} // namespace
