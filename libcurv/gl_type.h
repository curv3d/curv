// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_TYPE_H
#define LIBCURV_GL_TYPE_H

#include <libcurv/value.h>
#include <ostream>

namespace curv {

// an array indexed by GL_Type::Base_Type
extern struct GL_Type_Info
{
    const char* name;
    int rank;
    int dim1;
    int dim2;
} gl_type_info_array[];

// GL data types
struct GL_Type
{
    enum class Base_Type : int
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
    Base_Type base_type_;
    constexpr GL_Type() : base_type_(Base_Type::Any) {}
    constexpr GL_Type(Base_Type bt) : base_type_(bt) {}
    static constexpr inline GL_Type Any() { return {Base_Type::Any}; }
    static constexpr inline GL_Type Bool() { return {Base_Type::Bool}; }
    static constexpr inline GL_Type Num() { return {Base_Type::Num}; }
    static constexpr inline GL_Type Vec(int n)
    {
        return {Base_Type(int(Base_Type::Vec2) + n - 2)};
    }
    static constexpr inline GL_Type Mat(int n)
    {
        return {Base_Type(int(Base_Type::Mat2) + n - 2)};
    }

    inline const GL_Type_Info& info() const
    {
        return gl_type_info_array[int(base_type_) + 1];
    }
    // is a number, a vector, or a matrix
    inline bool is_numeric() const { return base_type_ >= Base_Type::Num; }
    inline bool is_list() const { return base_type_ >= Base_Type::Vec2; }
    inline int count() const { return info().dim1; }
    inline bool is_vec() const { return info().rank == 1; }
    inline bool is_mat() const { return info().rank == 2; }
    inline bool operator==(GL_Type rhs) const
    {
        return base_type_ == rhs.base_type_;
    }
    inline bool operator!=(GL_Type rhs) const
    {
        return base_type_ != rhs.base_type_;
    }
};

inline const GL_Type_Info& gl_type_info(GL_Type t)
{
    return t.info();
}

// is a number, a vector, or a matrix
inline bool gl_type_numeric(GL_Type type)
{
    return type.is_numeric();
}
inline bool gl_type_is_vec(GL_Type type)
{
    return type.is_vec();
}
inline bool gl_type_is_mat(GL_Type type)
{
    return type.is_mat();
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
    return GL_Type(GL_Type::Base_Type(len));
}
std::ostream& operator<<(std::ostream& out, GL_Type);

GL_Type gl_type_of(Value);

// The smallest type that includes both t1 and t2.
GL_Type gl_type_join(GL_Type t1, GL_Type t2);

} // namespace
#endif // header guard
