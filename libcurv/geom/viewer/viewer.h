// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <libcurv/filesystem.h>

namespace curv { namespace geom {

struct Shape_Recognizer;

namespace viewer {

int viewer_main(int, const char**);

struct Viewer
{
    Filesystem::path fragname_;

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
};

}}} // namespace
#endif // header guard
