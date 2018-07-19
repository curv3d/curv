// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <glm/glm.hpp>
#include <gl/shader.h>
#include <gl/vbo.h>

namespace curv { namespace geom {

struct Shape_Recognizer;

namespace viewer {

struct Viewer
{
    /*--- PUBLIC API ---*/
    Viewer();

    // Set the current shape. Must be called before calling run().
    virtual void set_shape(Shape_Recognizer&);

    // Initialize OpenGL, create the window.
    void open();

    // Called repeatedly in a loop once the window is open.
    // Returns false when it is time to exit the loop (user has closed window).
    bool draw_frame();

    // Close window, if not already closed. Set is_open to false.
    void close();

    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user. It is equivalent to:
    //     open();
    //     while (draw_frame());
    //     close();
    void run();

    /*--- PARAMETER STATE, can set before window is opened ---*/
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};

    /*--- SUBCLASS API ---*/

    // Called at start of each frame. If false, then frame loop exits.
    virtual bool next_frame();
    // Can be called from next_frame() to change the frag shader.
    void set_frag(const std::string&);

    /*--- INTERNAL STATE ---*/

    std::string fragsrc_{};
    Shader shader_{};
    std::string vertSource_{};
    std::vector<std::string> defines_{};
    bool verbose_{false};
    GLFWwindow* window_ = nullptr;
    Vbo* vbo_ = nullptr;

    glm::mat3 u_view2d_ = glm::mat3(1.);
    // These are the 'view3d' uniforms.
    // Note: the up3d vector must be orthogonal to (eye3d - centre3d),
    // or rotation doesn't work correctly.
    glm::vec3 u_centre3d_ = glm::vec3(0.,0.,0.);
    // The following initial value for 'eye3d' is derived by starting with
    // [0,0,6], then rotating 30 degrees around the X and Y axes.
    glm::vec3 u_eye3d_ = glm::vec3(2.598076,3.0,4.5);
    // The initial value for up3d is derived by starting with [0,1,0], then
    // applying the same rotations as above, so that up3d is orthogonal to eye3d.
    glm::vec3 u_up3d_ = glm::vec3(-0.25,0.866025,-0.433013);

    typedef struct {
        float     x,y;
        int       button;
        glm::vec2 velocity;
        glm::vec2 drag;
    } Mouse;
    Mouse mouse_;

    // INTERNAL FUNCTIONS
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
    void setup();
    void draw();
    void onKeyPress(int);
    void onMouseMove(double, double);
    void onScroll(float);
    void onMouseDrag(float, float, int);
    void debounceSetWindowTitle(std::string);
    void renderGL();
    void updateGL();
    void closeGL();
    float getPixelDensity();
    void onExit();
    void setWindowSize(int, int);
};

}}} // namespace
#endif // header guard
