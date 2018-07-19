// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#include <libcurv/geom/viewer/app.h>
#include <libcurv/geom/viewer/viewer.h>

#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace curv { namespace geom { namespace viewer {

// Common global variables
//----------------------------------------------------
const std::string appTitle = "curv";
static bool left_mouse_button_down = false;
static glm::ivec4 viewport;
static double fTime = 0.0f;
static double fFPS = 0.0f;
static float fPixelDensity = 1.0;

void Viewer::initGL (glm::ivec4 &_viewport, bool _headless)
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
        if (button == GLFW_MOUSE_BUTTON_1) {
            if (action == GLFW_PRESS && !left_mouse_button_down) {
                left_mouse_button_down = true;
            } else if (action == GLFW_RELEASE && left_mouse_button_down) {
                left_mouse_button_down = false;
            }
        }
        if (action == GLFW_PRESS) {
            self->mouse_.drag.x = self->mouse_.x;
            self->mouse_.drag.y = self->mouse_.y;
        }
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* win, double xoffset, double yoffset) {
        Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
        self->onScroll(-yoffset * fPixelDensity);
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
        if (fPixelDensity != self->getPixelDensity()) {
            self->setWindowSize(viewport.z, viewport.w);
        }
    });

    glfwSwapInterval(1);
}

void Viewer::onMouseMove(double x, double y)
{
    // Convert x,y to pixel coordinates relative to viewport.
    // (0,0) is lower left corner.
    y = viewport.w - y;
    x *= fPixelDensity;
    y *= fPixelDensity;
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
    if (mouse_.x > viewport.z * fPixelDensity) mouse_.x = viewport.z * fPixelDensity;
    if (mouse_.y > viewport.w * fPixelDensity) mouse_.y = viewport.w * fPixelDensity;

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

void Viewer::updateGL()
{
    // Update time
    // --------------------------------------------------------------------
    double now = glfwGetTime();
    double fDelta = now - fTime;
    fTime = now;

    static int frame_count = 0;
    static double lastTime = 0.0;
    frame_count++;
    lastTime += fDelta;
    if (lastTime >= 1.) {
        fFPS = double(frame_count);
        frame_count = 0;
        lastTime -= 1.;
    }

    // EVENTS
    std::string title = appTitle + " FPS:" + std::to_string(fFPS);
    debounceSetWindowTitle(title);
    glfwPollEvents();
}

void Viewer::renderGL()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // TODO FIXME
    glfwSwapBuffers(window_);
}

void Viewer::closeGL()
{
    //glfwSetWindowShouldClose(window_, GL_TRUE);
    glfwDestroyWindow(window_);
}
//-------------------------------------------------------------

void Viewer::setWindowSize(int _width, int _height)
{
    viewport.z = _width;
    viewport.w = _height;
    fPixelDensity = getPixelDensity();
    glViewport(0.0, 0.0, (float)getWindowWidth(), (float)getWindowHeight());
}

float Viewer::getPixelDensity()
{
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    return float(framebuffer_width)/float(window_width);
}

int getWindowWidth()
{
    return viewport.z*fPixelDensity;
}

int getWindowHeight()
{
    return viewport.w*fPixelDensity;
}

double getTime()
{
    return fTime;
}

}}}
