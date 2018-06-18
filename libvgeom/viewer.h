// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_VIEWER_H
#define LIBVGEOM_VIEWER_H

namespace vgeom {

struct Shape_Recognizer;

// Open a Viewer window displaying shape, and block until the window is closed.
void run_viewer(Shape_Recognizer& shape);

} // namespace
#endif // header guard
