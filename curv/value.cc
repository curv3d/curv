// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/value.h>
#include <aux/dtostr.h>

using namespace curv;

void
curv::Value::print(std::ostream& out)
const
{
    if (is_null()) {
        out << "null";
    } else if (is_bool()) {
        out << (get_bool_unsafe() ? "true" : "false");
    } else if (is_num()) {
        out << aux::dfmt(get_num_unsafe());
    } else if (is_ref()) {
        Ref_Value& ref(get_ref_unsafe());
        switch (ref.type_) {
        case Ref_Value::ty_string:
            out << "<string>";
            break;
        case Ref_Value::ty_list:
            out << "<list>";
            break;
        case Ref_Value::ty_object:
            out << "<object>";
            break;
        case Ref_Value::ty_function:
            out << "<function>";
            break;
        default:
            out << "<" << ref.type_ << ">";
            break;
        }
    } else {
        out << "???";
    }
}
