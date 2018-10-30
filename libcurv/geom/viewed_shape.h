// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_VIEWED_SHAPE_H
#define LIBCURV_GEOM_VIEWED_SHAPE_H

#include <libcurv/geom/frag.h>
#include <libcurv/geom/shape.h>

namespace curv {
namespace geom {

// Viewed_Shape is a representation of a Shape that is directly usable by
// the Viewer. It encodes the frag program, uniform variables, and pickers.
// It is self contained, containing no references to data owned by the
// evaluator thread.
struct Viewed_Shape
{
    std::string frag_;

    // This creates an empty Viewed_Shape (contains no shape).
    Viewed_Shape() {};

    // This creates a non-empty Viewed_Shape (contains a viewable shape).
    Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts);

    bool empty() const { return frag_.empty(); }
};

}} // namespace
#endif // header guard

