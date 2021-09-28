// Copyright 2016-2021 Doug Moen
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
        if (n > 0) {
            auto t = sc_type_of(ls->front());
            if (t) return SC_Type::List(t, n);
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
    return {t ? t->elem_type_ : Type::Error};
}

SC_Type
SC_Type::List(SC_Type etype, unsigned n)
{
    return {make<List_Type>(n, etype.type_)};
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
