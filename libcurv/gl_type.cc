// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/gl_type.h>

namespace curv {

GL_Type_Attr gl_types[] =
{
    {"bool",  0, 1, 1},
    {"float", 0, 1, 1},
    {"vec2",  1, 2, 1},
    {"vec3",  1, 3, 1},
    {"vec4",  1, 4, 1},
    {"mat2",  2, 2, 2},
    {"mat3",  2, 3, 3},
    {"mat4",  2, 4, 4},
};

std::ostream& operator<<(std::ostream& out, GL_Type type)
{
    return out << gl_type_name(type);
}

} // namespace curv
