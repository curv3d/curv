// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_TYPE_H
#define LIBCURV_GL_TYPE_H

#include <ostream>

namespace curv {

/// GL data types
enum class GL_Type : unsigned
{
    Bool = 0,
    Num = 1,
    Vec2 = 2,
    Vec3 = 3,
    Vec4 = 4,
    Mat2 = 5,
    Mat3 = 6,
    Mat4 = 7
};
extern struct GL_Type_Attr
{
    const char* name;
    int rank;
    int dim1;
    int dim2;
} gl_types[];
// is a number, a vector, or a matrix
inline bool gl_type_numeric(GL_Type type)
{
    return type >= GL_Type::Num;
}
inline bool gl_type_is_vec(GL_Type type)
{
    return gl_types[unsigned(type)].rank == 1;
}
inline bool gl_type_is_mat(GL_Type type)
{
    return gl_types[unsigned(type)].rank == 2;
}
// if numeric, how many numbers are stored.
inline unsigned gl_type_count(GL_Type type)
{
    GL_Type_Attr &ta = gl_types[unsigned(type)];
    return ta.dim1 * ta.dim2;
}
inline const char* gl_type_name(GL_Type type)
{
    return gl_types[(int)type].name;
}
inline GL_Type gl_vec_type(unsigned len /* range 2..4 */)
{
    return (GL_Type)len;
}
std::ostream& operator<<(std::ostream& out, GL_Type);

} // namespace
#endif // header guard
