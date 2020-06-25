// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/sc_type.h>

#include <libcurv/die.h>
#include <libcurv/reactive.h>

namespace curv {

std::ostream& operator<<(std::ostream& out, SC_Type type)
{
    out << type.plex_array_base().glsl_name();
    switch (type.plex_array_rank()) {
    case 1:
        out << "[" << type.plex_array_dim(0) << "]";
        break;
    case 2:
        out << "[" << type.plex_array_dim(0)
                   << "*" << type.plex_array_dim(1) << "]";
        break;
    }
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
                // Try to upgrade to a larger numeric plex.
                if (ty.is_num()) {
                    if (n >= 2 && n <= 4)
                        return SC_Type::Num(n);
                }
                else if (ty.is_num_vec()) {
                    if (n == ty.count())
                        return SC_Type::Mat(n);
                }
                // Try to construct a general numeric array type.
                if (ty.rank_ < 2) {
                    ty.type_ = make<List_Type>(n, ty.type_);
                    ++ty.rank_;
                    ty.dim2_ = ty.dim1_;
                    ty.dim1_ = n;
                    return ty;
                }
            }
            else if (ty.is_bool_tensor()) {
                // Try to upgrade to a larger boolean plex.
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
                    ty.type_ = make<List_Type>(n, ty.type_);
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
SC_Type::elem_type() const
{
    auto t = cast<const List_Type>(type_);
    Shared<const Type> et = t ? t->elem_type_ : Type::Error;
    switch (rank_) {
    case 0:
        if (is_num_vec())
            return Num();
        if (is_mat())
            return Num(count());
        if (base_type_ == Base_Type::Bool32)
            return Bool();
        if (base_type_ >= Base_Type::Bool2 && base_type_ <= Base_Type::Bool4)
            return Bool();
        if (base_type_ >= Base_Type::Bool2x32
            && base_type_ <= Base_Type::Bool4x32)
            return Bool32();
        return {};
    case 1:
        return {et, base_type_};
    case 2:
        return {et, base_type_, dim2_};
    default:
        die("SC_Type::elem_type() bad rank");
    }
}

SC_Type
SC_Type::List(SC_Type etype, unsigned n)
{
    auto lt = make<List_Type>(n, etype.type_);
    switch (etype.rank_) {
    case 0:
        if (etype.is_num()) {
            if (n >= 2 && n <= 4) return Num(n);
        } else if (etype.is_num_vec()) {
            if (etype.count() == n) return Mat(n);
        } else if (etype.is_bool()) {
            if (n == 32) return Bool32();
            if (n >= 2 && n <= 4) return Bool(n);
        } else if (etype.is_bool32()) {
            if (n >= 2 && n <= 4) return Bool32(n);
        }
        return SC_Type{lt, etype.base_type_, n};
    case 1:
        return SC_Type{lt, etype.base_type_, n, etype.dim1_};
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
        return sc_unified_list_type(a.elem_type(), b.elem_type(), a.count());
    }
    else if (a.is_list())
        return sc_unified_list_type(a.elem_type(), b, a.count());
    else if (b.is_list())
        return sc_unified_list_type(a, b.elem_type(), b.count());
    return {};
}

} // namespace curv
