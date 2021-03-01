// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/value.h>
#include <cmath>

namespace curv {

void help(Value val, std::ostream& out)
{
    if (val.is_missing()) {
        out <<
           "<missing>: "
           "This pseudo-value is used internally by the runtime system\n"
           "as a flag. It can't be returned by a Curv expression.\n"
           "You have found a bug in Curv. Please file a bug report.\n";
    } else if (val.is_bool()) {
        out << val << " is a Boolean value, and is also a symbol.\n";
    } else if (val.is_char()) {
        out << val
            << " is a character -- a single element of a character string.\n";
    } else if (val.is_num()) {
        double n = val.to_num_unsafe();
        if (std::isinf(n)) {
            if (n > 0)
                out << "inf is infinity: the largest positive number.\n";
            else
                out << "-inf is minus infinity: the smallest negative number.\n";
        } else {
            out << val << " is a number.\n";
        }
    } else if (val.is_ref()) {
        val.to_ref_unsafe().print_help(out);
    } else {
        out << "??? This value is unknown. You have found a bug in Curv.\n";
    }
}

void Ref_Value::print_help(std::ostream& out) const
{
    out << "There is no help yet for " << Value{share(*this)} << "\n";
}

void Closure::print_help(std::ostream& out) const
{
    out << "function " << Value{share(*this)} << "\n";
    out << "parameter: " << pattern_->syntax_->location().range() << "\n";
}

void Piecewise_Function::print_help(std::ostream& out) const
{
    out << "piecewise function, with these parameter signatures:\n";
    for (auto c : cases_) {
        out << "-- ";
        if (auto f = cast<const Closure>(c))
            out << f->pattern_->syntax_->location().range();
        else
            out << "unknown parameter signature for " << c;
        out << "\n";
    }
}

} // namespace
