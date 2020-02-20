// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GLSL_H
#define LIBCURV_GLSL_H

#include <ostream>

namespace curv {

struct Shape_Program;

// GLSL library functions
extern const char glsl_header[];

// Export a shape's dist and colour functions as a set of GLSL definitions.
void glsl_function_export(const Shape_Program&, std::ostream&);

} // namespace
#endif // header guard
