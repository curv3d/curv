// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_TYPE_H
#define LIBCURV_GL_TYPE_H

#include <libcurv/value.h>
#include <ostream>

namespace curv {

/// GL data types
enum class GL_Type : int
{
    Any = -1,
    Bool = 0,
    Num = 1,
    Vec2 = 2,
    Vec3 = 3,
    Vec4 = 4,
    Mat2 = 5,
    Mat3 = 6,
    Mat4 = 7
};

extern struct GL_Type_Info
{
    const char* name;
    int rank;
    int dim1;
    int dim2;
} gl_type_info_array[];
inline const GL_Type_Info& gl_type_info(GL_Type t)
{
    return gl_type_info_array[int(t) + 1];
}

// is a number, a vector, or a matrix
inline bool gl_type_numeric(GL_Type type)
{
    return type >= GL_Type::Num;
}
inline bool gl_type_is_vec(GL_Type type)
{
    return gl_type_info(type).rank == 1;
}
inline bool gl_type_is_mat(GL_Type type)
{
    return gl_type_info(type).rank == 2;
}
// if numeric, how many numbers are stored.
inline unsigned gl_type_count(GL_Type type)
{
    const GL_Type_Info &ta = gl_type_info(type);
    return ta.dim1 * ta.dim2;
}
inline const char* gl_type_name(GL_Type type)
{
    return gl_type_info(type).name;
}
inline GL_Type gl_vec_type(unsigned len /* range 2..4 */)
{
    return (GL_Type)len;
}
std::ostream& operator<<(std::ostream& out, GL_Type);

GL_Type gl_type_of(Value);

// The smallest type that includes both t1 and t2.
GL_Type gl_type_join(GL_Type t1, GL_Type t2);

} // namespace
#endif // header guard
