// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/viewed_shape.h>

namespace curv {
namespace geom {

Viewed_Shape::Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts)
{
    std::stringstream frag;
    export_frag(shape, opts, frag);
    frag_ = frag.str();
}

}} // namespace
