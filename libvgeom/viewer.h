// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_VIEWER_H
#define LIBVGEOM_VIEWER_H

namespace vgeom {

struct Shape_Recognizer;

// Open a Viewer window displaying shape, and block until the window is closed.
void run_viewer(Shape_Recognizer& shape);

// Open a Viewer window, if one is not already open. Display 'shape' in that
// window, if possible, then return as soon as the shape is visible.
void open_viewer(Shape_Recognizer& shape);

// Close the Viewer window created by open_viewer(), if it exists.
void close_viewer();

} // namespace
#endif // header guard
