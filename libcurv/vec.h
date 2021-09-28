// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_VEC_H
#define LIBCURV_VEC_H

#include <libcurv/value.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace curv {

struct Context;

using Vec2 = glm::dvec2;
using Vec3 = glm::dvec3;
bool unbox_vec2(Value, Vec2&);
Vec3 value_to_vec3(Value, const Context&);

} // namespace
#endif // header guard
