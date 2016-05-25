// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/value.h>

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
        out << get_num_unsafe();
    } else {
        out << "???";
    }
}
