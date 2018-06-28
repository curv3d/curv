// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause
#include <libcurv/geom/viewer/app.h>

#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
#include "tools/text.h"

namespace curv { namespace geom { namespace viewer {

// Common global variables
//----------------------------------------------------
const std::string appTitle = "curv";
static glm::mat4 orthoMatrix;
typedef struct {
    float     x,y;
    int       button;
    float     velX,velY;
    glm::vec2 drag;
} Mouse;
struct timeval tv;
static Mouse mouse;
static bool left_mouse_button_down = false;
static glm::vec4 iMouse = {0.0, 0.0, 0.0, 0.0};
static glm::ivec4 viewport;
static double fTime = 0.0f;
static double fDelta = 0.0f;
static double fFPS = 0.0f;
static float fPixelDensity = 1.0;
GLFWwindow* window;

void initGL (glm::ivec4 &_viewport, bool _headless)
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

    window = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), NULL, NULL);

    if(!window) {
        glfwTerminate();
        std::cerr << "ABORT: GLFW create window failed" << std::endl;
        exit(-1);
    }

    setWindowSize(_viewport.z, _viewport.w);

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _w, int _h) {
        setWindowSize(_w,_h);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
        onKeyPress(_key);
    });

    // callback when a mouse button is pressed or released
    glfwSetMouseButtonCallback(window, [](GLFWwindow* _window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            // update iMouse when left mouse button is pressed or released
            if (action == GLFW_PRESS && !left_mouse_button_down) {
                left_mouse_button_down = true;
                iMouse.x = mouse.x;
                iMouse.y = mouse.y;
                iMouse.z = mouse.x;
                iMouse.w = mouse.y;
            } else if (action == GLFW_RELEASE && left_mouse_button_down) {
                left_mouse_button_down = false;
                iMouse.z = -iMouse.z;
                iMouse.w = -iMouse.w;
            }
        }
        if (action == GLFW_PRESS) {
            mouse.drag.x = mouse.x;
            mouse.drag.y = mouse.y;
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow* _window, double xoffset, double yoffset) {
        onScroll(-yoffset * fPixelDensity);
    });

    // callback when the mouse cursor moves
    glfwSetCursorPosCallback(window, [](GLFWwindow* _window, double x, double y) {
        // Convert x,y to pixel coordinates relative to viewport.
        // (0,0) is lower left corner.
        y = viewport.w - y;
        x *= fPixelDensity;
        y *= fPixelDensity;
        // mouse.velX,mouse.velY is the distance the mouse cursor has moved
        // since the last callback, during a drag gesture.
        // mouse.drag is the previous mouse position, during a drag gesture.
        // Note that mouse.drag is *not* constrained to the viewport.
        mouse.velX = x - mouse.drag.x;
        mouse.velY = y - mouse.drag.y;
        mouse.drag.x = x;
        mouse.drag.y = y;

        // mouse.x,mouse.y is the current cursor position, constrained
        // to the viewport.
        mouse.x = x;
        mouse.y = y;
        if (mouse.x < 0) mouse.x = 0;
        if (mouse.y < 0) mouse.y = 0;
        if (mouse.x > viewport.z * fPixelDensity) mouse.x = viewport.z * fPixelDensity;
        if (mouse.y > viewport.w * fPixelDensity) mouse.y = viewport.w * fPixelDensity;

        // update iMouse when cursor moves
        if (left_mouse_button_down) {
            iMouse.x = mouse.x;
            iMouse.y = mouse.y;
        }

        /*
         * TODO: the following code would best be moved into the
         * mouse button callback. If you click the mouse button without
         * moving the mouse, then using this code, the mouse click doesn't
         * register until the cursor is moved. (@doug-moen)
         */
        int action1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
        int action2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
        int button = 0;

        if (action1 == GLFW_PRESS) button = 1;
        else if (action2 == GLFW_PRESS) button = 2;

        // Lunch events
        if (mouse.button == 0 && button != mouse.button) {
            mouse.button = button;
            onMouseClick(mouse.x,mouse.y,mouse.button);
        }
        else {
            mouse.button = button;
        }

        if (mouse.velX != 0.0 || mouse.velY != 0.0) {
            if (button != 0) onMouseDrag(mouse.x,mouse.y,mouse.button);
            else onMouseMove(mouse.x,mouse.y);
        }
    });

    glfwSetWindowPosCallback(window, [](GLFWwindow* _window, int x, int y) {
        if (fPixelDensity != getPixelDensity()) {
            setWindowSize(viewport.z, viewport.w);
        }
    });

    glfwSwapInterval(1);
}

bool isGL(){
    return !glfwWindowShouldClose(window);
}

void debounceSetWindowTitle(std::string title){
    static double lastUpdated;

    double now = glfwGetTime();

    if ((now - lastUpdated) < 1.) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());

    lastUpdated = now;
}

void updateGL(){
    // Update time
    // --------------------------------------------------------------------
    double now = glfwGetTime();
    fDelta = now - fTime;
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
    std::string title = appTitle + " FPS:" + toString(fFPS);
    debounceSetWindowTitle(title);
    glfwPollEvents();
}

void renderGL(){
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    glfwSwapBuffers(window);
}

void closeGL(){
    //glfwSetWindowShouldClose(window, GL_TRUE);
    glfwDestroyWindow(window);
}
//-------------------------------------------------------------

void setWindowSize(int _width, int _height) {
    viewport.z = _width;
    viewport.w = _height;
    fPixelDensity = getPixelDensity();
    glViewport(0.0, 0.0, (float)getWindowWidth(), (float)getWindowHeight());
    orthoMatrix = glm::ortho((float)viewport.x, (float)getWindowWidth(), (float)viewport.y, (float)getWindowHeight());

    onViewportResize(getWindowWidth(), getWindowHeight());
}

glm::ivec2 getScreenSize() {
    glm::ivec2 screen;
    glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &screen.x, &screen.y);
    return screen;
}

float getPixelDensity() {
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    return float(framebuffer_width)/float(window_width);
}

glm::ivec4 getViewport() {
    return viewport;
}

int getWindowWidth() {
    return viewport.z*fPixelDensity;
}

int getWindowHeight() {
    return viewport.w*fPixelDensity;
}

glm::mat4 getOrthoMatrix() {
    return orthoMatrix;
}

glm::vec4 getDate() {
    gettimeofday(&tv, NULL);
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    // std::cout << "y: " << tm->tm_year+1900 << " m: " << tm->tm_mon << " d: " << tm->tm_mday << " s: " << tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001 << std::endl;
    return glm::vec4(tm->tm_year+1900,
                     tm->tm_mon,
                     tm->tm_mday,
                     tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001);
}

double getTime() {
    return fTime;
}

double getDelta() {
    return fDelta;
}

double getFPS() {
    return fFPS;
}

float getMouseX(){
    return mouse.x;
}

float getMouseY(){
    return mouse.y;
}

glm::vec2 getMousePosition() {
    return glm::vec2(mouse.x,mouse.y);
}

float getMouseVelX(){
    return mouse.velX;
}

float getMouseVelY(){
    return mouse.velY;
}

glm::vec2 getMouseVelocity() {
    return glm::vec2(mouse.velX,mouse.velY);
}

int getMouseButton(){
    return mouse.button;
}

glm::vec4 get_iMouse() {
    return iMouse;
}

}}}
