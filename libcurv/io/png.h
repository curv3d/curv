// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_PNG_H
#define LIBCURV_GEOM_PNG_H

#include <libcurv/render.h>
#include <glm/vec2.hpp>

namespace curv {
struct Output_File;
struct Shape_Program;

namespace geom {

// Image export parameters
struct Image_Export : public Render_Opts
{
    Image_Export() { aa_ = 4; }
    glm::ivec2 size;    // Size of exported image, in pixels.
    double pixel_size;  // Size of a square pixel, in shape space.
    double fstart_ = 0.0;  // Frame start time, in seconds, for animations.
    bool verbose_ = false;
};

void export_png(const Shape_Program&, const Image_Export&, Output_File&);

}} // namespace
#endif // header guard
