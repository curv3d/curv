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
    unsigned rank;
    unsigned dim1;
    unsigned dim2;
} gl_type_info_array[];

// GL data types
struct GL_Type
{
    enum class Base_Type : short
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

    // 4 shorts == 64 bit representation
    Base_Type base_type_;
    // rank 0: The type is just 'base_type_'.
    // rank 1: The type is array of base_type_.
    // rank 2: A 2D array of base_type, represented in GLSL as a 1D array,
    //         since we target GLSL 1.5, which doesn't support multi-D arrays.
    unsigned short rank_ = 0;
    unsigned short dim1_ = 0;
    unsigned short dim2_ = 0;

    constexpr GL_Type() : base_type_(Base_Type::Any) {}
    constexpr GL_Type(Base_Type bt, unsigned rank = 0, unsigned dim1 = 0)
    :
        base_type_(bt),
        rank_(rank),
        dim1_(dim1)
    {}
    static constexpr inline GL_Type Any() { return {Base_Type::Any}; }
    static constexpr inline GL_Type Bool() { return {Base_Type::Bool}; }
    static constexpr inline GL_Type Num(unsigned dim1 = 0)
    {
        if (dim1 != 0)
            return {Base_Type::Num, 1, dim1};
        else
            return {Base_Type::Num};
    }
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
    inline unsigned count() const { return info().dim1; }
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

// if numeric, how many numbers are stored.
inline unsigned gl_type_count(GL_Type type)
{
    const GL_Type_Info &ta = type.info();
    return ta.dim1 * ta.dim2;
}
std::ostream& operator<<(std::ostream& out, GL_Type);

GL_Type gl_type_of(Value);

// The smallest type that includes both t1 and t2.
GL_Type gl_type_join(GL_Type t1, GL_Type t2);

} // namespace
#endif // header guard
