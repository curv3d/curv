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
    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user.
    void run();

    /*--- SUBCLASS API ---*/

    // Called at start of each frame. If false, then frame loop exits.
    virtual bool next_frame();
    // Can be called from next_frame() to change the frag shader.
    void set_frag(const std::string&);
    // Called after frame loop exits and window is closed.
    virtual void on_close();

    /*--- SHARED STATE ---*/

    std::string fragsrc_{};
    Shader shader_{};
    std::string vertSource_{};
    std::vector<std::string> defines_{};
    bool verbose_{false};

    /*--- PARAMETER STATE, can set before thread is started ---*/
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};

    static int main(Viewer*); // Viewer thread entry point
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
    void setup();
    void draw();
};

}}} // namespace
#endif // header guard
