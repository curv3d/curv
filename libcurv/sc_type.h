// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_TYPE_H
#define LIBCURV_SC_TYPE_H

#include <libcurv/value.h>
#include <libcurv/type.h>
#include <ostream>

namespace curv {

// an array indexed by SC_Type::Base_Type
extern const struct SC_Base_Type_Info
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
    static constexpr unsigned MAX_MAT_COUNT = 4; // max first dim of a matrix
    enum class Base_Type : short
    {
        Error = -1,
        Bool,
        Bool2,
        Bool3,
        Bool4,
        Bool32,
        Bool2x32,
        Bool3x32,
        Bool4x32,
        Num,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4
    };

private:
    /* New Representation */
    Shared<const Type> type_;

    /* Old Representation */
    // 4 shorts == 64 bit representation
    Base_Type base_type_;
    // rank 0: The type is just 'base_type_'.
    // rank 1: The type is array of base_type_.
    // rank 2: A 2D array of base_type, represented in GLSL as a 1D array,
    //         since we target GLSL 1.5, which doesn't support multi-D arrays.
    unsigned short rank_ = 0;
    unsigned short dim1_ = 0;
    unsigned short dim2_ = 0;

public:
    friend std::ostream& operator<<(std::ostream& out, SC_Type type);
    friend SC_Type sc_type_of(Value v);

    SC_Type() : type_(), base_type_(Base_Type::Error) {}
private:
    SC_Type(Shared<const Type> t, Base_Type bt,
            unsigned dim1 = 0, unsigned dim2 = 0)
    :
        type_(t),
        base_type_(bt),
        rank_(dim2 ? 2 : dim1 ? 1 : 0),
        dim1_(dim1),
        dim2_(dim2)
    {}
public:
    static inline SC_Type Error() { return {}; }
    static inline SC_Type Bool(unsigned n = 1)
    {
        if (n == 1)
            return {Type::Bool, Base_Type::Bool};
        assert(n >= 2 && n <= 4);
        return {make<List_Type>(n, Type::Bool),
                Base_Type(int(Base_Type::Bool) + n-1)};
    }
    static inline SC_Type Bool32(unsigned n=1)
    {
        if (n == 1)
            return {Type::Bool32, Base_Type::Bool32};
        assert(n >= 2 && n <= 4);
        return {make<List_Type>(n, Type::Bool32),
                Base_Type(int(Base_Type::Bool32) + n-1)};
    }
    static inline SC_Type Num(unsigned n = 1)
    {
        if (n == 1)
            return {Type::Num, Base_Type::Num};
        assert(n >= 2 && n <= 4);
        return {make<List_Type>(n, Type::Num),
                Base_Type(int(Base_Type::Num) + n-1)};
    }
    static inline SC_Type Vec(SC_Type base, unsigned n)
    {
        assert(base.rank_ == 0 &&
            (base.base_type_ == Base_Type::Bool ||
             base.base_type_ == Base_Type::Num ||
             base.base_type_ == Base_Type::Bool32));
        assert(n >= 1 && n <= 4);
        return {make<List_Type>(n, base.type_),
                Base_Type(int(base.base_type_) + n-1)};
    }
    static inline SC_Type Mat(int n)
    {
        assert(n >= 2 && n <= 4);
        return {make<List_Type>(n, make<List_Type>(n, Type::Num)),
                Base_Type(int(Base_Type::Mat2) + n - 2)};
    }
    static SC_Type List(SC_Type etype, unsigned n);

private:
    inline const SC_Base_Type_Info& base_info() const
    {
        return sc_base_type_info_array[int(base_type_) + 1];
    }
public:

    /*
     * Boolean predicates
     */
    // Is a single Bool value. Consistent with Value::is_bool().
    inline bool is_bool() const
    {
        return base_type_ == Base_Type::Bool
            && rank_ == 0;
    }
    // Is a single Bool or vector of Bool (count 2-4). Not an array.
    inline bool is_bool_or_vec() const
    {
        return base_type_ >= Base_Type::Bool
            && base_type_ <= Base_Type::Bool4
            && rank_ == 0;
    }
    // Is a single Bool32. Not an array.
    inline bool is_bool32() const
    {
        return base_type_ == Base_Type::Bool32
            && rank_ == 0;
    }
    // Is a single Bool32 or vector of Bool32. Not an array.
    inline bool is_bool32_or_vec() const
    {
        return base_type_ >= Base_Type::Bool32
            && base_type_ <= Base_Type::Bool4x32
            && rank_ == 0;
    }
    // Is a bool, a bool vec, a bool32, or a bool32 vec. Not an array.
    inline bool is_bool_plex() const
    {
        return base_type_ >= Base_Type::Bool
            && base_type_ <= Base_Type::Bool4x32
            && rank_ == 0;
    }
    // Is a bool, a bool vec, a bool32, a bool32 vec, or array of same.
    inline bool is_bool_tensor() const
    {
        return base_type_ >= Base_Type::Bool
            && base_type_ <= Base_Type::Bool4x32;
    }

    /*
     * Numeric predicates
     */
    // Is a single number. Conforms to Value::is_num().
    inline bool is_num() const
    {
        return base_type_ == Base_Type::Num
            && rank_ == 0;
    }
    // is a single number or vector
    inline bool is_num_or_vec() const
    {
        return base_type_ >= Base_Type::Num
            && base_type_ <= Base_Type::Vec4
            && rank_ == 0;
    }
    // Is a single number, vector, or matrix. Not an array.
    inline bool is_num_plex() const
    {
        return base_type_ >= Base_Type::Num
            && base_type_ <= Base_Type::Mat4
            && rank_ == 0;
    }
    // Is a number, a vector, a matrix, or an array of same.
    inline bool is_num_tensor() const
    {
        return base_type_ >= Base_Type::Num
            && base_type_ <= Base_Type::Mat4;
    }
    inline bool is_num_vec() const
    {
        return rank_ == 0
            && base_type_ >= Base_Type::Vec2 && base_type_ <= Base_Type::Vec4;
    }
    inline bool is_mat() const
    {
        return rank_ == 0
            && base_type_ >= Base_Type::Mat2 && base_type_ <= Base_Type::Mat4;
    }

    // These functions view an SC_Type as a multi-D array of plexes.
    // If plex_array_rank()==0 then the type is a plex.
    inline unsigned plex_array_rank() const { return rank_; }
    inline SC_Type plex_array_base() const {
        auto t = type_;
        while (t && t->plex_type_ == Plex_Type::missing)
            t = cast<const List_Type>(t)->elem_type_;
        return SC_Type(t, base_type_);
    }
    inline int plex_array_dim(int i) const {
        if (i == 0) return dim1_;
        else if (i == 1) return dim2_;
        else return 0; }

    // a Plex type is one of the following 3 mutually exclusive cases:
    inline bool is_scalar_plex() const { return rank_ == 0; }
    inline bool is_tuple() const { return false; }
    inline bool is_struct() const { return false; }

    inline bool is_plex() const { return rank_ == 0; }
    inline bool is_list() const { return rank() > 0; }
    inline bool is_scalar_or_vec() const {
        return rank_ == 0 && base_info().rank <= 1 && base_info().dim1 <= 4;
    }
    inline bool is_vec() const
    {
        return rank_ == 0 && (
            (base_type_ >= Base_Type::Bool2 && base_type_ <= Base_Type::Bool4)
            || (base_type_ >= Base_Type::Bool2x32
                && base_type_ <= Base_Type::Bool4x32)
            || (base_type_ >= Base_Type::Vec2 && base_type_ <= Base_Type::Vec4)
        );
    }

    // number of dimensions: 0 means a scalar (Num or Bool or Error)
    inline unsigned rank() const
    {
        return rank_ + base_info().rank;
    }
    // First dimension, if type is a list, or 1 if type is a scalar.
    inline unsigned count() const
    {
        if (rank_)
            return dim1_;
        return base_info().dim1;
    }
    // If this is an array, strip one dimension off of the type.
    SC_Type elem_type() const;

    inline bool operator==(SC_Type rhs) const
    {
        bool new_eq = Type::equal(type_, rhs.type_);
        bool old_eq = base_type_ == rhs.base_type_ && rank_ == rhs.rank_
            && dim1_ == rhs.dim1_ && dim2_ == rhs.dim2_;
        assert(old_eq == new_eq);
        return new_eq;
    }
    inline bool operator!=(SC_Type rhs) const
    {
        return !(*this == rhs);
    }
    explicit operator bool () const noexcept
    {
        return base_type_ != Base_Type::Error;
    }
};

std::ostream& operator<<(std::ostream& out, SC_Type);

SC_Type sc_type_of(Value);

SC_Type sc_unify_tensor_types(SC_Type a, SC_Type b);

} // namespace
#endif // header guard
