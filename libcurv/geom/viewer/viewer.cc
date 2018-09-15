// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#include <libcurv/geom/viewer/viewer.h>

//#include <time.h>
//#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

#include <libcurv/string.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <types/shapes.h>

namespace curv { namespace geom { namespace viewer {

Viewer::Viewer()
{
}

Viewer::Viewer(const Viewer_Config& config)
:
    config_(config)
{
}

void
Viewer::set_shape(const Shape_Program& shape)
{
    set_shape(shape, config_);
}

void
Viewer::set_shape(const Shape_Program& shape, const Frag_Export& opts)
{
    std::stringstream f;
    export_frag(shape, opts, f);
    fragsrc_ = f.str();
    if (is_open()) {
        shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
        shader_.load(fragsrc_, vertSource_, config_.verbose_);
    }
}

void
Viewer::run()
{
    open();
    while (draw_frame());
    close();
}

void Viewer::set_frag(const std::string& fragSource)
{
    shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    shader_.load(fragSource, vertSource_, config_.verbose_);
}

void Viewer::open()
{
    if (!is_open()) {
        // Set initial default values for centre, eye and up
        reset_view();

        // Initialize openGL context
        initGL(window_pos_and_size_, headless_);

        // Start working on the GL context
        setup();
    }
}

void Viewer::reset_view()
{
    u_view2d_ = glm::mat3(1.);
    u_centre3d_ = glm::vec3(0.,0.,0.);
    u_eye3d_ = glm::vec3(2.598076,3.0,4.5);
    u_up3d_ = glm::vec3(-0.25,0.866025,-0.433013);
}

bool Viewer::draw_frame()
{
    if (glfwWindowShouldClose(window_))
        return false;
    render();
    swap_buffers();
    poll_events();
    if (!config_.lazy_)
        measure_time();
    return true;
}

void Viewer::close()
{
    if (is_open()) {
        //glfwGetWindowPos(window_,
        //    &window_pos_and_size_.x, &window_pos_and_size_.y);
        glfwGetWindowSize(window_,
            &window_pos_and_size_.w, &window_pos_and_size_.z);
        onExit();
    }
}

Viewer::~Viewer()
{
    close();
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
    shader_.load(fragsrc_, vertSource_, config_.verbose_);

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Viewer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.use();

    // Pass uniforms
    shader_.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    if (shader_.needTime()) {
        shader_.setUniform("u_time", float(current_time_));
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
    else if (_key == 'r' || _key == 'R') {
        reset_view();
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

const std::string appTitle = "curv";

void Viewer::initGL(glm::ivec4 &_viewport, bool _headless)
{
    glfwSetErrorCallback([](int err, const char* msg)->void {
        std::cerr << "GLFW error 0x"<<std::hex<<err<<std::dec<<": "<<msg<<"\n";
    });
    if(!glfwInit()) {
        std::cerr << "ABORT: GLFW init failed" << std::endl;
        exit(-1);
    }

    if (_headless) {
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    }

    window_ = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), NULL, NULL);

    if(!window_) {
        glfwTerminate();
        std::cerr << "ABORT: GLFW create window failed" << std::endl;
        exit(-1);
    }
    glfwSetWindowUserPointer(window_, (void*)this);

    setWindowSize(_viewport.z, _viewport.w);
    glfwSetWindowPos(window_, _viewport.x, _viewport.y);

    glfwMakeContextCurrent(window_);
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* win, int w, int h) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->setWindowSize(w,h);
    });

    glfwSetKeyCallback(window_, [](GLFWwindow* win, int _key, int _scancode, int _action, int _mods) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->onKeyPress(_key);
    });

    // callback when a mouse button is pressed or released
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* win, int button, int action, int mods) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        if (action == GLFW_PRESS) {
            self->mouse_.drag.x = self->mouse_.x;
            self->mouse_.drag.y = self->mouse_.y;
        }
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* win, double xoffset, double yoffset) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->onScroll(-yoffset * self->fPixelDensity_);
    });

    // callback when the mouse cursor moves
    glfwSetCursorPosCallback(window_, [](GLFWwindow* win, double x, double y) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->onMouseMove(x, y);
    });

    glfwSetWindowPosCallback(window_, [](GLFWwindow* win, int x, int y) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->window_pos_and_size_.x = x;
        self->window_pos_and_size_.y = y;
        if (self->fPixelDensity_ != self->getPixelDensity()) {
            self->setWindowSize(self->viewport_.z, self->viewport_.w);
        }
    });

    glfwSwapInterval(1);
}

void Viewer::onMouseMove(double x, double y)
{
    // Convert x,y to pixel coordinates relative to viewport.
    // (0,0) is lower left corner.
    y = viewport_.w - y;
    x *= fPixelDensity_;
    y *= fPixelDensity_;
    // mouse_.velocity is the distance the mouse cursor has moved
    // since the last callback, during a drag gesture.
    // mouse_.drag is the previous mouse position, during a drag gesture.
    // Note that mouse_.drag is *not* constrained to the viewport.
    mouse_.velocity.x = x - mouse_.drag.x;
    mouse_.velocity.y = y - mouse_.drag.y;
    mouse_.drag.x = x;
    mouse_.drag.y = y;

    // mouse_.x,mouse_.y is the current cursor position, constrained
    // to the viewport.
    mouse_.x = x;
    mouse_.y = y;
    if (mouse_.x < 0) mouse_.x = 0;
    if (mouse_.y < 0) mouse_.y = 0;
    if (mouse_.x > viewport_.z * fPixelDensity_) mouse_.x = viewport_.z * fPixelDensity_;
    if (mouse_.y > viewport_.w * fPixelDensity_) mouse_.y = viewport_.w * fPixelDensity_;

    /*
     * TODO: the following code would best be moved into the
     * mouse button callback. If you click the mouse button without
     * moving the mouse, then using this code, the mouse click doesn't
     * register until the cursor is moved. (@doug-moen)
     */
    int action1 = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_1);
    int action2 = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_2);
    int button = 0;

    if (action1 == GLFW_PRESS) button = 1;
    else if (action2 == GLFW_PRESS) button = 2;

    if (mouse_.button == 0 && button != mouse_.button) {
        mouse_.button = button;
    }
    else {
        mouse_.button = button;
    }

    if (mouse_.velocity.x != 0.0 || mouse_.velocity.y != 0.0) {
        if (button != 0) onMouseDrag(mouse_.x,mouse_.y,mouse_.button);
    }
}

void Viewer::debounceSetWindowTitle(std::string title)
{
    static double lastUpdated;

    double now = glfwGetTime();

    if ((now - lastUpdated) < 1.) {
        return;
    }

    glfwSetWindowTitle(window_, title.c_str());

    lastUpdated = now;
}

void Viewer::measure_time()
{
    // Update time
    // --------------------------------------------------------------------
    double now = glfwGetTime();
    double fDelta = now - current_time_;
    current_time_ = now;

    static int frame_count = 0;
    static double lastTime = 0.0;
    frame_count++;
    lastTime += fDelta;
    if (lastTime >= 1.) {
        fFPS_ = double(frame_count);
        frame_count = 0;
        lastTime -= 1.;
    }

    // EVENTS
    std::string title = appTitle + " FPS:" + std::to_string(fFPS_);
    debounceSetWindowTitle(title);
}

void Viewer::poll_events()
{
    if (config_.lazy_)
        glfwWaitEvents();
    else
        glfwPollEvents();
}

void Viewer::swap_buffers()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // TODO FIXME
    glfwSwapBuffers(window_);
}

void Viewer::closeGL()
{
    //glfwSetWindowShouldClose(window_, GL_TRUE);
    glfwDestroyWindow(window_);
    window_ = nullptr;
}

void Viewer::setWindowSize(int _width, int _height)
{
    viewport_.z = _width;
    viewport_.w = _height;
    fPixelDensity_ = getPixelDensity();
    glViewport(0.0, 0.0, (float)getWindowWidth(), (float)getWindowHeight());
}

float Viewer::getPixelDensity()
{
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    return float(framebuffer_width)/float(window_width);
}

int Viewer::getWindowWidth()
{
    return viewport_.z*fPixelDensity_;
}

int Viewer::getWindowHeight()
{
    return viewport_.w*fPixelDensity_;
}

}}} // namespace
