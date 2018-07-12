// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef SHAPES_H
#define SHAPES_H

#include <libcurv/geom/shape.h>
#include <string>

void print_shape(curv::geom::Shape_Recognizer& shape);
std::string shape_to_frag(curv::geom::Shape_Recognizer& shape);

#endif // include guard
