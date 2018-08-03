// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/export_png.h>

#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewer/viewer.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/output_file.h>

#include <gl/texture.h>

namespace curv { namespace geom {

void
export_png(
    const Shape_Program& shape,
    glm::ivec2 size,
    double pixsize,
    Output_File& ofile)
{
    glm::dvec2 shape_size = shape.bbox_.size2();
    glm::dvec2 image_coverage = glm::dvec2(size) * pixsize;
    glm::dvec2 overpaint = image_coverage - shape_size;
    glm::dvec2 origin = {
        shape.bbox_.xmin - overpaint.x/2.0 + pixsize/2.0,
        shape.bbox_.ymax + overpaint.y/2.0 - pixsize/2.0,
    };

    viewer::Viewer v;
    v.window_pos_and_size_.z = size.x;
    v.window_pos_and_size_.w = size.y;
    v.headless_ = true;
    v.set_shape(shape);
    v.open();
    v.draw_frame();
    unsigned char* pixels = new unsigned char[size.x*size.y*4];
    glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    v.close();

    Texture::savePixels(ofile.path().c_str(), pixels, size.x, size.y);
}

}} // namespace
