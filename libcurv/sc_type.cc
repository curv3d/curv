// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/sc_type.h>
#include <libcurv/math.h>
#include <libcurv/reactive.h>

namespace curv {

SC_Base_Type_Info sc_base_type_info_array[] =
{
    {"Any",   0, 1, 1},
    {"bool",  0, 1, 1},
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
    if (v.is_bool())
        return SC_Type::Bool();
    if (v.is_num())
        return SC_Type::Num();
    if (isvec3(v))
        return SC_Type::Vec(3);
    if (auto re = v.dycast<Reactive_Value>())
        return re->sctype_;
    return SC_Type::Any();
}

SC_Type
sc_type_join(SC_Type t1, SC_Type t2)
{
    if (t1 == t2) return t1;
    return SC_Type::Any();
}

} // namespace curv
