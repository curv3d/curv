// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/meanings.h>

#include <typeinfo>
#include <boost/core/demangle.hpp>

namespace curv
{

void Operation::print_repr(std::ostream& out, Prec) const
{
    out << "<" << boost::core::demangle(typeid(*this).name()) << ">";
}
void Constant::print_repr(std::ostream& out, Prec rprec) const
{
    value_.print_repr(out, rprec);
}
void Call_Expr::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    func_->print_repr(out, Prec::postfix);
    out << " ";
    arg_->print_repr(out, Prec::primary);
    close_paren(out, rprec, Prec::postfix);
}
void Index_Expr::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    arg1_->print_repr(out, Prec::postfix);
    out << "@";
    arg2_->print_repr(out, Prec::primary);
    close_paren(out, rprec, Prec::postfix);
}
void Slice_Expr::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    arg1_->print_repr(out, Prec::postfix);
    out << ".";
    arg2_->print_repr(out, Prec::primary);
    close_paren(out, rprec, Prec::postfix);
}
void List_Expr_Base::print_repr(std::ostream& out, Prec) const
{
    out << "[";
    for (size_t i = 0; i < size(); ++i) {
        if (i != 0) out << ",";
        array_[i]->print_repr(out, Prec::item);
    }
    out << "]";
}
void If_Else_Op::print_repr(std::ostream& out, Prec rprec) const
{
    // TODO: `if` print_repr: follows grammar, but wrong. grammar ambiguous.
    open_paren(out, rprec, Prec::ritem);
    out << "if (" << *arg1_ << ") ";
    arg2_->print_repr(out, Prec::ritem);
    out << " else ";
    arg3_->print_repr(out, Prec::ritem);
    close_paren(out, rprec, Prec::ritem);
}

} // namespace curv
