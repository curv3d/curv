// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_EXPORT_PNG_H
#define LIBCURV_GEOM_EXPORT_PNG_H

#include <glm/vec2.hpp>

namespace curv {
struct Output_File;

namespace geom {
struct Shape_Program;

// Image export parameters
struct Image_Export
{
    glm::ivec2 size;    // Size of exported image, in pixels.
    double pixel_size;  // Size of a square pixel, in shape space.
    double time = 0.0;  // Time stamp in seconds of shape, for animations.
    int aa = 1;         // Supersampling factor for antialiasing.
};

void export_png(const Shape_Program&, const Image_Export&, Output_File&);

}} // namespace
#endif // header guard
