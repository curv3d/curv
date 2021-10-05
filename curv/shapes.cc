// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "shapes.h"

#include <libcurv/function.h>
#include <sstream>
#include <iostream>

template <class SHAPE>
void
print_shape_generic(
    const SHAPE& shape)
{
    if (shape.is_2d_) std::cerr << "2D";
    if (shape.is_2d_ && shape.is_3d_) std::cerr << "/";
    if (shape.is_3d_) std::cerr << "3D";
    std::cerr << " shape "
        << (shape.bbox_.max.x - shape.bbox_.min.x) << "*"
        << (shape.bbox_.max.y - shape.bbox_.min.y);
    if (shape.is_3d_)
        std::cerr << "*" << (shape.bbox_.max.z - shape.bbox_.min.z);
    std::cerr << "\n";
}

void
print_shape(
    const curv::Shape_Program& shape)
{
    print_shape_generic(shape);
}

void
print_shape(
    const curv::GPU_Program& shape)
{
    print_shape_generic(shape);
}
