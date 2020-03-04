// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/sc_type.h>

#include <libcurv/die.h>
#include <libcurv/reactive.h>

namespace curv {

const SC_Base_Type_Info sc_base_type_info_array[] =
{
    {"Error", 0, 1, 1},
    {"bool",  0, 1, 1},
    {"bvec2", 1, 2, 1},
    {"bvec3", 1, 3, 1},
    {"bvec4", 1, 4, 1},
    {"uint",  1, 32, 1},
    {"uvec2", 2, 2, 32},
    {"uvec3", 2, 3, 32},
    {"uvec4", 2, 4, 32},
    {"float", 0, 1, 1},
    {"vec2",  1, 2, 1},
    {"vec3",  1, 3, 1},
    {"vec4",  1, 4, 1},
    {"mat2",  2, 2, 2},
    {"mat3",  2, 3, 3},
    {"mat4",  2, 4, 4},
};

std::ostream& operator<<(std::ostream& out, SC_Type type)
{
    out << type.base_info().name;
    if (type.rank_ == 1)
        out << "[" << type.dim1_ << "]";
    else if (type.rank_ == 2)
        out << "[" << type.dim1_ << "*" << type.dim2_ << "]";
    return out;
}

SC_Type
sc_type_of(Value v)
{
    if (v.is_num())
        return SC_Type::Num();
    else if (v.is_bool())
        return SC_Type::Bool();
    else if (auto ls = v.maybe<List>()) {
        auto n = ls->size();
        if (n == 0 || n > SC_Type::MAX_LIST)
            ;
        else {
            auto ty = sc_type_of(ls->front());
            if (ty.is_num_tensor()) {
                // Try to upgrade to a larger numeric struc.
                if (ty.is_num()) {
                    if (n >= 2 && n <= 4)
                        return SC_Type::Vec(n);
                }
                else if (ty.is_num_vec()) {
                    if (n == ty.count())
                        return SC_Type::Mat(n);
                }
                // Try to construct a general numeric array type.
                if (ty.rank_ < 2) {
                    ++ty.rank_;
                    ty.dim2_ = ty.dim1_;
                    ty.dim1_ = n;
                    return ty;
                }
            }
            else if (ty.is_bool_tensor()) {
                // Try to upgrade to a larger boolean struc.
                if (ty.is_bool()) {
                    if (n >= 2 && n <= 4)
                        return SC_Type::Bool(n);
                    if (n == 32)
                        return SC_Type::Bool32();
                }
                else if (ty.is_bool32()) {
                    if (n >= 2 && n <= 4)
                        return SC_Type::Bool32(n);
                }
                // Try to construct a general boolean array type.
                if (ty.rank_ < 2) {
                    ++ty.rank_;
                    ty.dim2_ = ty.dim1_;
                    ty.dim1_ = n;
                    return ty;
                }
            }
        }
    }
    else if (auto re = v.maybe<Reactive_Value>())
        return re->sctype_;
    return SC_Type::Error();
}

SC_Type
SC_Type::abase() const
{
    switch (rank_) {
    case 0:
        if (is_num_vec())
            return Num();
        if (is_mat())
            return Vec(count());
        if (base_type_ == Base_Type::Bool32)
            return Bool();
        if (base_type_ >= Base_Type::Bool2 && base_type_ <= Base_Type::Bool4)
            return Bool();
        if (base_type_ >= Base_Type::Bool2x32
            && base_type_ <= Base_Type::Bool4x32)
            return Bool32();
        return *this;
    case 1:
        return {base_type_};
    case 2:
        return {base_type_, dim2_};
    default:
        die("SC_Type::abase() bad rank");
    }
}

SC_Type
SC_Type::List(SC_Type etype, unsigned n)
{
    switch (etype.rank_) {
    case 0:
        if (etype.is_num()) {
            if (n >= 2 && n <= 4) return Vec(n);
        } else if (etype.is_num_vec()) {
            if (etype.count() == n) return Mat(n);
        } else if (etype.is_bool()) {
            if (n == 32) return Bool32();
            if (n >= 2 && n <= 4) return Bool(n);
        } else if (etype.is_bool32()) {
            if (n >= 2 && n <= 4) return Bool32(n);
        }
        return SC_Type{etype.base_type_, n};
    case 1:
        return SC_Type{etype.base_type_, n, etype.dim1_};
    default:
        die("SC_Type::List() bad rank");
    }
}

SC_Type sc_unified_list_type(SC_Type a, SC_Type b, unsigned n)
{
    SC_Type etype = sc_unify_tensor_types(a, b);
    if (etype) return SC_Type::List(etype, n); else return {};
}

SC_Type sc_unify_tensor_types(SC_Type a, SC_Type b)
{
    if (a == b) { return a; }
    if (a.is_list() && b.is_list()) {
        if (a.count() != b.count()) return {};
        return sc_unified_list_type(a.abase(), b.abase(), a.count());
    }
    else if (a.is_list())
        return sc_unified_list_type(a.abase(), b, a.count());
    else if (b.is_list())
        return sc_unified_list_type(a, b.abase(), b.count());
    return {};
}

} // namespace curv
