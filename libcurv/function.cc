// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/exception.h>
#include <libcurv/frame.h>
#include <libcurv/context.h>
#include <libcurv/meanings.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <typeinfo>
#include <boost/core/demangle.hpp>

namespace curv {

Shared<const Function>
maybe_function(Value funv, const Context& cx)
{
    static Symbol_Ref callkey = make_symbol("call");
    for (;;) {
        if (auto func = funv.maybe<const Function>())
            return func;
        if (auto rec = funv.maybe<const Record>()) {
            if (rec->hasfield(callkey)) {
                funv = rec->getfield(callkey, cx);
                continue;
            }
        }
        return nullptr;
    }
}

Shared<const Function>
value_to_function(Value funv, Fail fl, const Context& cx)
{
    auto fun = maybe_function(funv, cx);
    if (fun != nullptr)
        return fun;
    FAIL(fl, nullptr, cx, stringify(funv," is not a function"));
}

const char Function::name[] = "function";

void
Function::print_repr(std::ostream& out, Prec) const
{
    out << "<function";
    if (fname_)
        out << " " << fname_;
    out << ">";
}

void
Function::tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    fm->result_ = call(arg, Fail::hard, *fm);
    fm->next_op_ = nullptr;
}

bool
Function::try_tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    Value result = call(arg, Fail::soft, *fm);
    if (!result.is_missing()) {
        fm->result_ = result;
        fm->next_op_ = nullptr;
        return true;
    }
    return false;
}

SC_Value
Function::sc_call_expr(Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& fm)
const
{
    throw Exception(At_SC_Phrase(func_part(call_phrase), fm),
        stringify("function class <",
            boost::core::demangle(typeid(*this).name()),
            "> is not supported"));
}

Value
Curried_Function::call(Value arg, Fail fl, Frame& fm) const
{
    if (!validate_arg(0, arg, fl, At_Arg(*this, fm)))
        return missing;
    Shared<Partial_Application> pa = make_tail_array<Partial_Application>
        ({arg}, nargs(), fname_.name_, share(*this));
    pa->fname_.argpos_ = 1;
    return {pa};
}
bool
Curried_Function::validate_arg(unsigned, Value, Fail, const At_Syntax&) const
{
    return true;
}

Value
Partial_Application_Base::call(Value arg, Fail fl, Frame& fm) const
{
    if (fname_.argpos_ == nargs() - 1) {
        for (unsigned i = 0; i < fname_.argpos_; ++i)
            fm[i] = array_[i];
        fm[fname_.argpos_] = arg;
        return cfunc_->ccall(*this, fl, fm);
    } else {
        Shared<Partial_Application> pa = make_tail_array<Partial_Application>
            (fname_.argpos_+1, nargs(), fname_.name_, cfunc_);
        for (unsigned i = 0; i < fname_.argpos_; ++i)
            pa->array_[i] = array_[i];
        if (!cfunc_->validate_arg(fname_.argpos_, arg, fl, At_Arg(*this, fm)))
            return missing;
        pa->array_[fname_.argpos_] = arg;
        pa->fname_.argpos_ = fname_.argpos_ + 1;
        return {pa};
    }
}

Value
Tuple_Function::call(Value arg, Fail fl, Frame& fm) const
{
    if (nargs_ == 1) {
        fm[0] = arg;
        return tuple_call(fl, fm);
    }
    At_Arg cx(*this, fm);
    auto list = arg.to<const Abstract_List>(cx);
    ASSERT_SIZE(fl,missing,list,nargs_,cx);
    for (size_t i = 0; i < list->size(); ++i)
        fm[i] = list->val_at(i);
    return tuple_call(fl, fm);
}

SC_Value
Tuple_Function::sc_call_expr(
    Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& fm)
const
{
    auto f2 = make_tail_array<SC_Frame>(nslots_, fm.sc_, nullptr, &fm,
        share(*this), call_phrase);
    if (nargs_ == 1)
        (*f2)[0] = sc_eval_op(fm, arg);
    else if (auto list = cast_list_expr(arg)) {
        if (list->size() != nargs_)
            throw Exception(At_SC_Phrase(arg.syntax_, fm), stringify(
                "wrong number of arguments (got ",list->size(),
                ", expected ",nargs_,")"));
        for (size_t i = 0; i < list->size(); ++i)
            (*f2)[i] = sc_eval_op(fm, *(*list)[i]);
    } else {
        auto scarg = sc_eval_op(fm, arg);
        if (!scarg.type.is_vec())
            throw Exception(At_SC_Phrase(arg.syntax_, fm), stringify(
                "function call argument is not a vector (has type ",
                scarg.type,")"));
        if (scarg.type.count() != nargs_)
            throw Exception(At_SC_Phrase(arg.syntax_, fm), stringify(
                "wrong number of arguments (got ",scarg.type.count(),
                ", expected ",nargs_,")"));
        for (unsigned i = 0; i < scarg.type.count(); ++i)
            (*f2)[i] = sc_vec_element(fm, scarg, i);
    }
    return sc_tuple_call(*f2);
}

SC_Value
Tuple_Function::sc_tuple_call(SC_Frame& fm) const
{
    throw Exception(At_SC_Phrase(func_part(fm.call_phrase_), fm),
        stringify("function class <",
            boost::core::demangle(typeid(*this).name()),
            "> is not supported"));
}

Value
Closure::call(Value arg, Fail fl, Frame& fm) const
{
    fm.nonlocals_ = &*nonlocals_;
    if (fl == Fail::soft) {
        if (!pattern_->try_exec(fm.array_, arg, At_Arg(*this, fm), fm))
            return missing;
    } else {
        pattern_->exec(fm.array_, arg, At_Arg(*this, fm), fm);
    }
    return expr_->eval(fm);
}

void
Closure::tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    fm->nonlocals_ = &*nonlocals_;
    pattern_->exec(fm->array_, arg, At_Arg(*this, *fm), *fm);
    fm->next_op_ = &*expr_;
}

bool
Closure::try_tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    fm->nonlocals_ = &*nonlocals_;
    if (!pattern_->try_exec(fm->array_, arg, At_Arg(*this, *fm), *fm))
        return false;
    fm->next_op_ = &*expr_;
    return true;
}

SC_Value
Closure::sc_call_expr(Operation& arg, Shared<const Phrase> cp, SC_Frame& fm) const
{
    // create a frame to call this closure
    auto f2 = make_tail_array<SC_Frame>(nslots_, fm.sc_, nullptr, &fm,
        share(*this), cp);
    f2->nonlocals_ = &*nonlocals_;
    // match pattern against argument, store formal parameters in frame
    pattern_->sc_exec(arg, fm, *f2);
    // evaluation function body, return result.
    return sc_eval_op(*f2, *expr_);
}

void
Lambda::print_repr(std::ostream& out, Prec) const
{
    out << "<lambda>";
}

slot_t
Piecewise_Function::maxslots(std::vector<Shared<const Function>>& cases)
{
    slot_t result = 0;
    for (auto c : cases)
        result = std::max(result, c->nslots_);
    return result;
}

Value
Piecewise_Function::call(Value arg, Fail fl, Frame& fm) const
{
    for (auto c : cases_) {
        Value result = c->call(arg, Fail::soft, fm);
        if (!result.is_missing())
            return result;
    }
    FAIL(fl, missing, At_Arg(*this, fm),
        stringify(arg," has no matching pattern"));
}

void
Piecewise_Function::tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    for (auto c : cases_) {
        if (c->try_tail_call(arg, fm))
            return;
    }
    throw Exception(At_Arg(*this, *fm), stringify(
        arg," has no matching pattern"));
}

bool
Piecewise_Function::try_tail_call(Value arg, std::unique_ptr<Frame>& fm) const
{
    for (auto c : cases_) {
        if (c->try_tail_call(arg, fm))
            return true;
    }
    return false;
}

SC_Value
Piecewise_Function::sc_call_expr(
    Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& fm)
const
{
    throw Exception(At_SC_Phrase(func_part(call_phrase), fm),
        "piecewise function is not supported");
}

slot_t
Composite_Function::maxslots(std::vector<Shared<const Function>>& cases)
{
    slot_t result = 0;
    for (auto c : cases)
        result = std::max(result, c->nslots_);
    return result;
}

Value
Composite_Function::call(Value arg, Fail fl, Frame& fm) const
{
    for (auto c : cases_) {
        arg = c->call(arg, fl, fm);
        if (arg.is_missing())
            break;
    }
    return arg;
}

SC_Value
Composite_Function::sc_call_expr(
    Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& fm)
const
{
    // There's no early exit if one of the functions fails.
    // Complex and costly to do, and with little benefit.
    auto val = sc_eval_op(fm, arg);
    for (auto c : cases_) {
        auto op = make<SC_Value_Expr>(call_phrase, val);
        val = c->sc_call_expr(*op, call_phrase, fm);
    }
    return val;
}

} // namespace curv
