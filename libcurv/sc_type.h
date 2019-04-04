// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_TYPE_H
#define LIBCURV_SC_TYPE_H

#include <libcurv/value.h>
#include <ostream>

namespace curv {

// an array indexed by SC_Type::Base_Type
extern struct SC_Base_Type_Info
{
    const char* name;
    unsigned rank;
    unsigned dim1;
    unsigned dim2;
} sc_base_type_info_array[];

// SC data types
struct SC_Type
{
    static constexpr unsigned MAX_LIST = 65535; // max size of an array dimension
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

    constexpr SC_Type() : base_type_(Base_Type::Any) {}
    constexpr SC_Type(Base_Type bt, unsigned dim1 = 0, unsigned dim2 = 0)
    :
        base_type_(bt),
        rank_(dim2 ? 2 : dim1 ? 1 : 0),
        dim1_(dim1),
        dim2_(dim2)
    {}
    static constexpr inline SC_Type Any() { return {Base_Type::Any}; }
    static constexpr inline SC_Type Bool() { return {Base_Type::Bool}; }
    static constexpr inline SC_Type Num(unsigned dim1 = 0, unsigned dim2 = 0)
    {
        return {Base_Type::Num, dim1, dim2};
    }
    static constexpr inline SC_Type Vec(
        int n, unsigned dim1 = 0, unsigned dim2 = 0)
    {
        return {Base_Type(int(Base_Type::Vec2) + n - 2), dim1, dim2};
    }
    static constexpr inline SC_Type Mat(int n)
    {
        return {Base_Type(int(Base_Type::Mat2) + n - 2)};
    }

    inline const SC_Base_Type_Info& base_info() const
    {
        return sc_base_type_info_array[int(base_type_) + 1];
    }
    // is a number, a vector, or a matrix
    inline bool is_numeric() const { return base_type_ >= Base_Type::Num; }
    inline bool is_list() const
    {
        return base_type_ >= Base_Type::Vec2 || rank_ > 0;
    }
    // number of dimensions: 0 means a scalar (Num or Bool or Any)
    inline unsigned rank() const
    {
        return rank_ + base_info().rank;
    }
    // first dimension, if type is a list
    inline unsigned count() const
    {
        if (rank_)
            return dim1_;
        return base_info().dim1;
    }
    // If this is an array, strip one dimension off of the type.
    SC_Type abase()
    {
        if (is_vec())
            return Num();
        return {base_type_};
    }
    inline bool is_vec() const
    {
        return rank_ == 0 && base_info().rank == 1;
    }
    inline bool is_mat() const
    {
        return rank_ == 0 && base_info().rank == 2;
    }
    inline bool operator==(SC_Type rhs) const
    {
        return base_type_ == rhs.base_type_ && rank_ == rhs.rank_
            && dim1_ == rhs.dim1_ && dim2_ == rhs.dim2_;
    }
    inline bool operator!=(SC_Type rhs) const
    {
        return !(*this == rhs);
    }
};

// if numeric, how many numbers are stored.
inline unsigned sc_type_count(SC_Type type)
{
    const SC_Base_Type_Info &ta = type.base_info();
    return ta.dim1 * ta.dim2;
}
std::ostream& operator<<(std::ostream& out, SC_Type);

SC_Type sc_type_of(Value);

// The smallest type that includes both t1 and t2.
SC_Type sc_type_join(SC_Type t1, SC_Type t2);

} // namespace
#endif // header guard
