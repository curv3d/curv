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

void export_png(const Shape_Program&, glm::ivec2, Output_File&);

}} // namespace
#endif // header guard
