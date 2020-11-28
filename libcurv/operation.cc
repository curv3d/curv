// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/meaning.h>

#include <typeinfo>
#include <boost/core/demangle.hpp>

namespace curv
{

void Operation::print(std::ostream& out) const
{
    out << "<" << boost::core::demangle(typeid(*this).name()) << ">";
}
void Constant::print(std::ostream& out) const
{
    out << value_;
}
void Call_Expr::print(std::ostream& out) const
{
    out << *func_ << " " << *arg_;
}
void Index_Expr::print(std::ostream& out) const
{
    out << *arg1_ << "@" << *arg2_;
}
void Slice_Expr::print(std::ostream& out) const
{
    out << *arg1_ << "." << *arg2_;
}
void List_Expr_Base::print(std::ostream& out) const
{
    out << "[";
    for (size_t i = 0; i < size(); ++i) {
        if (i != 0) out << ",";
        out << *array_[i];
    }
    out << "]";
}
void If_Else_Op::print(std::ostream& out) const
{
    out << "(if (" << *arg1_ << ") " << *arg2_ << " else " << *arg3_ << ")";
}

} // namespace curv
