// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <libcurv/filesystem.h>
#include <thread>
#include <mutex>
#include <glm/glm.hpp>
#include <gl/shader.h>

namespace curv { namespace geom {

struct Shape_Recognizer;

namespace viewer {

struct Viewer
{
    /*--- PUBLIC API ---*/

    // Set the current shape. Must be called before calling run().
    virtual void set_shape(Shape_Recognizer&);

    // Called repeatedly in a loop once the window is open.
    // Returns false when it is time to exit the loop (user has closed window).
    bool draw_frame();

    // Close window, if not already closed. Set is_open to false.
    void close();

    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user.
    void run();

    /*--- SUBCLASS API ---*/

    // Called at start of each frame. If false, then frame loop exits.
    virtual bool next_frame();
    // Can be called from next_frame() to change the frag shader.
    void set_frag(const std::string&);

    /*--- SHARED STATE ---*/

    std::string fragsrc_{};
    Shader shader_{};
    std::string vertSource_{};
    std::vector<std::string> defines_{};
    bool verbose_{false};
    GLFWwindow* window_ = nullptr;

    /*--- PARAMETER STATE, can set before thread is started ---*/
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};

    static int main(Viewer*); // Viewer thread entry point
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
    void setup();
    void draw();
    void onKeyPress(int);
    void onMouseMove(double, double);
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
