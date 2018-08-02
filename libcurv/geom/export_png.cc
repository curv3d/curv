// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/export_png.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/output_file.h>
#include <libcurv/geom/shape.h>

namespace curv { namespace geom {

void
export_png(const Shape_Program& shape, glm::ivec2 size, Output_File& ofile)
{
    (void) shape;
    (void) size;
    (void) ofile;
    throw Exception({}, "PNG export not implemented");
}

}} // namespace
