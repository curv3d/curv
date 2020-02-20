// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/value.h>

#include <libcurv/dtostr.h>
#include <libcurv/exception.h>
#include <libcurv/list.h>
#include <libcurv/reactive.h>
#include <libcurv/record.h>
#include <libcurv/string.h>
#include <libcurv/typeconv.h>

#include <climits>
#include <cmath>

namespace curv {

bool
Value::to_bool(const Context& cx) const
{
    if (is_bool())
        return to_bool_unsafe();
    throw Exception(cx, stringify(*this, " is not a boolean"));
}

double
Value::to_num(const Context& cx) const
{
    if (is_num())
        return to_num_unsafe();
    throw Exception(cx, stringify(*this, " is not a number"));
}

bool
Value::is_int() const noexcept
{
    if (number_ != number_) return false;
    double intf;
    return modf(number_, &intf) == 0.0
        && intf >= double(INT_MIN)
        && intf <= double(INT_MAX);
}

int
Value::to_int(int lo, int hi, const Context& cx) const
{
    if (!is_num())
        throw Exception(cx, stringify(*this, " is not a number"));
    return num_to_int(to_num_unsafe(), lo, hi, cx);
}

void
Value::to_abort [[noreturn]] (const Context& cx, const char* type) const
{
    throw Exception(cx, stringify(*this, " is not a ",type));
}

Value
Value::at(Symbol_Ref field, const Context& cx) const
{
    if (is_ref()) {
        Record* s = dynamic_cast<Record*>(&to_ref_unsafe());
        if (s)
            return s->getfield(field, cx);
    }
    throw Exception(cx, stringify(*this," does not contain field .",field));
}

void
Value::print(std::ostream& out)
const
{
    if (is_missing()) {
        out << "<missing>";
    } else if (is_bool()) {
        out << (to_bool_unsafe() ? "#true" : "#false");
    } else if (is_num()) {
        out << dfmt(to_num_unsafe());
    } else if (is_ref()) {
        to_ref_unsafe().print(out);
    } else {
        out << "???";
    }
}

bool Value::equal(Value v, const Context& cx) const
{
    // Numeric equality is the fast path, so it is handled first.
    // If both are numbers, this computes floating point equality:
    // true if they are the same number, with -0==+0.
    // If v is not a number, then it's NaN, so we'll return false.
    if (is_num())
        return number_ == v.number_;

    if (!is_ref()) {
        // *this is a non-numeric immediate value: boolean or null.
        return bits_ == v.bits_;
    }

    // at this point, *this is a reference value.

    if (!v.is_ref())
        return false;

    // both are reference values
    const Ref_Value& r1{to_ref_unsafe()};
    const Ref_Value& r2{v.to_ref_unsafe()};

    if (r1.type_ != r2.type_)
        return false;

    // two reference values with the same type
    switch (r1.type_) {
    case Ref_Value::ty_symbol:
        return (Symbol&)r1 == (Symbol&)r2;
    case Ref_Value::ty_string:
        return (String&)r1 == (String&)r2;
    case Ref_Value::ty_list:
        return ((List&)r1).equal((List&)r2, cx);
    case Ref_Value::ty_record:
        return ((Record&)r1).equal((Record&)r2, cx);
    default:
        // Outside of the 6 data types, two values are equal if they have
        // the same type.
        return true;
    }
}

size_t Value::hash() const noexcept
{
    auto re = this->dycast<const Reactive_Expression>();
    if (re) return re->hash();
    return bits_;
}

bool Value::hash_eq(Value rhs) const noexcept
{
    if (bits_ == rhs.bits_) return true;
    auto re = this->dycast<const Reactive_Expression>();
    if (re) {
        auto re2 = rhs.dycast<const Reactive_Expression>();
        if (re2)
            return re->hash_eq(*re2);
    }
    return false;
}

const Value missing{};

} // namespace curv
