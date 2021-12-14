// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_TYPE_H
#define LIBCURV_SC_TYPE_H

#include <libcurv/value.h>
#include <libcurv/type.h>
#include <ostream>

namespace curv {

// SC data types
struct SC_Type
{
    static constexpr unsigned MAX_MAT_COUNT = 4; // max first dim of a matrix

public:
    Shared<const Type> type_;   // never null
    SC_Type(Shared<const Type> t) : type_(t) {}
public:
    SC_Type() : type_(Type::Error) {}
    static inline SC_Type Error() { return {}; }
    static inline SC_Type Bool(unsigned n = 1) {
        if (n == 1)
            return {Type::Bool};
        assert(n >= 2 && n <= 4);
        return {make<Array_Type>(n, Type::Bool)};
    }
    static inline SC_Type Bool32(unsigned n=1) {
        if (n == 1)
            return {Type::Bool32};
        assert(n >= 2 && n <= 4);
        return {make<Array_Type>(n, Type::Bool32)};
    }
    static inline SC_Type Num(unsigned n = 1) {
        if (n == 1)
            return {Type::Num};
        assert(n >= 2 && n <= 4);
        return {make<Array_Type>(n, Type::Num)};
    }
    static inline SC_Type Vec(SC_Type base, unsigned n) {
      #if !defined(NDEBUG)
        auto plex = base.type_->plex_type_;
      #endif
        assert(plex == Plex_Type::Bool ||
               plex == Plex_Type::Num ||
               plex == Plex_Type::Bool32);
        assert(n >= 1 && n <= 4);
        return {make<Array_Type>(n, base.type_)};
    }
    static inline SC_Type Mat(int n) {
        assert(n >= 2 && n <= 4);
        return {make<Array_Type>(n, make<Array_Type>(n, Type::Num))};
    }
    static SC_Type List(SC_Type etype, unsigned n);

public:
    inline bool is_error() const {
        return type_->subtype_ == Ref_Value::sty_error_type;
    }

    /*
     * Boolean predicates
     */
    // Is a single Bool value. Consistent with Value::is_bool().
    inline bool is_bool() const {
        return type_->plex_type_ == Plex_Type::Bool;
    }
    // Is a single Bool or vector of Bool (count 2-4). Not an array.
    inline bool is_bool_or_vec() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Bool && plex <= Plex_Type::Bool4;
    }
    // Is a single Bool32. Not an array.
    inline bool is_bool32() const {
        return type_->plex_type_ == Plex_Type::Bool32;
    }
    // Is a single Bool32 or vector of Bool32. Not an array.
    inline bool is_bool32_or_vec() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Bool32 && plex <= Plex_Type::Bool4x32;
    }
    // Is a bool, a bool vec, a bool32, or a bool32 vec. Not an array.
    inline bool is_bool_plex() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Bool && plex <= Plex_Type::Bool4x32;
    }
    // Is a bool, a bool vec, a bool32, a bool32 vec, or array of same.
    inline bool is_bool_tensor() const {
        auto plex = type_->plex_array_base()->plex_type_;
        return plex >= Plex_Type::Bool && plex <= Plex_Type::Bool4x32;
    }

    /*
     * Numeric predicates
     */
    // Is a single number. Conforms to Value::is_num().
    inline bool is_num() const {
        return type_->plex_type_ == Plex_Type::Num;
    }
    // is a single number or vector
    inline bool is_num_or_vec() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Num && plex <= Plex_Type::Vec4;
    }
    // Is a single number, vector, or matrix. Not an array.
    inline bool is_num_plex() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Num && plex <= Plex_Type::Mat4;
    }
    // Is a number, a vector, a matrix, or an array of same.
    inline bool is_num_tensor() const {
        auto plex = type_->plex_array_base()->plex_type_;
        return plex >= Plex_Type::Num && plex <= Plex_Type::Mat4;
    }
    inline bool is_num_vec() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Vec2 && plex <= Plex_Type::Vec4;
    }
    inline bool is_mat() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Mat2 && plex <= Plex_Type::Mat4;
    }

    // These functions view an SC_Type as a multi-D array of plexes.
    // If plex_array_rank()==0 then the type is a plex.
    inline unsigned plex_array_rank() const {
        return type_->plex_array_rank();
    }
    inline SC_Type plex_array_base() const {
        return SC_Type(type_->plex_array_base());
    }
    inline unsigned plex_array_dim(unsigned i) const {
        return type_->plex_array_dim(i);
    }
    inline const char* glsl_name() const {
        return glsl_plex_type_name[unsigned(type_->plex_type_)];
    }

    // a Plex type is one of the following 3 mutually exclusive cases:
    inline bool is_scalar_plex() const {
        return type_->plex_type_ != Plex_Type::missing;
    }
    inline bool is_tuple() const { return false; }
    inline bool is_struct() const { return false; }

    inline bool is_plex() const {
        return type_->plex_type_ != Plex_Type::missing;
    }
    inline bool is_list() const {
        return type_->subtype_ == Ref_Value::sty_array_type;
    }
    inline bool is_scalar_or_vec() const {
        auto plex = type_->plex_type_;
        return plex >= Plex_Type::Bool && plex <= Plex_Type::Vec4;
    }
    inline bool is_vec() const {
        auto plex = type_->plex_type_;
        return (plex >= Plex_Type::Bool2 && plex <= Plex_Type::Bool4)
            || (plex >= Plex_Type::Bool2x32 && plex <= Plex_Type::Bool4x32)
            || (plex >= Plex_Type::Vec2 && plex <= Plex_Type::Vec4);
    }

    // number of dimensions: 0 means a scalar (Num or Bool or Error)
    inline unsigned rank() const {
        return type_->rank();
    }
    // First dimension, if type is a list, or 1 if type is a scalar.
    inline unsigned count() const {
        return type_->subtype_ == Ref_Value::sty_array_type
            ? ((Array_Type*)(&*type_))->count_
            : 1;
    }
    // If this is an array, strip one dimension off of the type.
    SC_Type elem_type() const;

    inline bool operator==(SC_Type rhs) const {
        return Type::equal(*type_, *rhs.type_);
    }
    inline bool operator!=(SC_Type rhs) const {
        return !(*this == rhs);
    }
    explicit operator bool () const noexcept {
        return type_->subtype_ != Ref_Value::sty_error_type;
    }
};

std::ostream& operator<<(std::ostream& out, SC_Type);

SC_Type sc_type_of(Value);

SC_Type sc_unify_tensor_types(SC_Type a, SC_Type b);

} // namespace
#endif // header guard
