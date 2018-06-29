// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <libcurv/filesystem.h>
#include <thread>
#include <mutex>
#include <glm/glm.hpp>

namespace curv { namespace geom {

struct Shape_Recognizer;

namespace viewer {

struct Viewer
{
    /*--- PUBLIC API ---*/

    // Set the current shape.
    void set_shape(Shape_Recognizer&);
    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user.
    void run();
    // Open a Viewer window, if one is not already open. Display the current
    // shape in that window, if possible, and return as soon as the shape
    // is visible.
    void open();
    // If a Viewer window is currently open (due to an open() call),
    // then close it.
    void close();
    ~Viewer();

    /*--- SHARED STATE ---*/

    // thread_: viewer thread runs OpenGL main loop.
    std::thread thread_{};
    // mutex_: for communication with viewer thread.
    std::mutex mutex_{};
    // is_open_: true if a viewer thread is running. Guarded by mutex_.
    // If true: A viewer thread is running and using the Viewer global state.
    //          Can change asynchronously to false if mutex_ not held.
    // If false: Either the viewer thread is not running, or the viewer thread
    //           is closing down and no longer accessing global state.
    //           Cannot change asynchronously from this state.
    bool is_open_{false};
    enum class Request {
        k_new_shape, // Shape has changed, please display it.
        k_close,     // Please close the Viewer window and stop the thread.
        k_none
    };
    // request_: If != k_none, then denotes an outstanding request that
    // the Viewer thread has not processed yet. Guarded by mutex_.
    Request request_{Request::k_none};
    // Name of fragment shader source file.
    Filesystem::path fragname_{};

    /*--- PARAMETER STATE, can set before thread is started ---*/
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};

    static int main(Viewer*); // Viewer thread entry point
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
};

}}} // namespace
#endif // header guard
