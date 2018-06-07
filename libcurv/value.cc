// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/value.h>
#include <libcurv/dtostr.h>
#include <libcurv/string.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/exception.h>

namespace curv {

void
Value::to_null(const Context& cx) const
{
    if (!is_null())
        throw Exception(cx, stringify("is not null: ", *this));
}

bool
Value::to_bool(const Context& cx) const
{
    if (is_bool())
        return get_bool_unsafe();
    throw Exception(cx, stringify("is not a boolean: ", *this));
}

double
Value::to_num(const Context& cx) const
{
    if (is_num())
        return get_num_unsafe();
    throw Exception(cx, stringify("is not a number: ", *this));
}

void
Value::to_abort [[noreturn]] (const Context& cx, const char* type)
{
    throw Exception(cx, stringify("is not a ",type,": ",*this));
}

Value
Value::at(Symbol field, const Context& cx) const
{
    if (is_ref()) {
        Structure* s = dynamic_cast<Structure*>(&get_ref_unsafe());
        if (s)
            return s->getfield(field, cx);
    }
    throw Exception(cx, stringify(".",field,": not defined"));
}

void
Value::print(std::ostream& out)
const
{
    if (is_null()) {
        out << "null";
    } else if (is_bool()) {
        out << (get_bool_unsafe() ? "true" : "false");
    } else if (is_num()) {
        out << dfmt(get_num_unsafe());
    } else if (is_ref()) {
        get_ref_unsafe().print(out);
    } else {
        out << "???";
    }
}

auto Value::operator==(Value v) const
-> bool
{
    // Numeric equality is the fast path, so it is handled first.
    // If both are numbers, this computes floating point equality:
    // true if they are the same number, with -0==+0.
    // If v is not a number, then it's NaN, so we'll return false.
    if (is_num())
        return number_ == v.number_;

    if (!is_ref()) {
        // *this is a non-numeric immediate value.
        return bits_ == v.bits_;
    }

    // at this point, *this is a reference value.

    if (!v.is_ref())
        return false;

    // both are reference values
    const Ref_Value& r1{get_ref_unsafe()};
    const Ref_Value& r2{v.get_ref_unsafe()};

    if (r1.type_ != r2.type_)
        return false;

    // two reference values with the same type
    switch (r1.type_) {
    case Ref_Value::ty_string:
        return (String&)r1 == (String&)r2;
    case Ref_Value::ty_list:
        return (List&)r1 == (List&)r2;
    case Ref_Value::ty_record:
        return (Record&)r1 == (Record&)r2;
    default:
        // Outside of the 6 data types, two values are equal if they have
        // the same type.
        return true;
    }
}

// special marker that denotes the absence of a value
struct Missing : public Ref_Value
{
    Missing() : Ref_Value(ty_missing) {}

    void print(std::ostream& out) const override
    {
        out << "<missing>";
    }
};
Value missing {make<Missing>()};

} // namespace curv
