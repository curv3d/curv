// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>

namespace curv {

Shared<const Function>
cast_to_function(Value funv, const Context& cx)
{
    static Symbol_Ref callkey = "call";
    static Symbol_Ref conskey = "constructor";
    for (;;) {
        auto func = funv.dycast<const Function>();
        if (func)
            return func;
        auto rec = funv.dycast<const Record>();
        if (rec->hasfield(callkey)) {
            funv = rec->getfield(callkey, cx);
            continue;
        }
        if (rec->hasfield(conskey)) {
            funv = rec->getfield(conskey, cx);
            continue;
        }
        return nullptr;
    }
}

const char Function::name[] = "function";

void
Function::print(std::ostream& out) const
{
    out << "<function";
    if (!name_.empty()) {
        out << " " << name_;
        for (int i = 0; i < argpos_; ++i)
            out << " _";
    }
    out << ">";
}

void
Function::tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    f->result_ = call(arg, *f);
    f->next_op_ = nullptr;
}

bool
Function::try_tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    Value result = try_call(arg, *f);
    if (!result.eq(missing)) {
        f->result_ = result;
        f->next_op_ = nullptr;
        return true;
    }
    return false;
}

SC_Value
Function::sc_call_expr(Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& f)
const
{
    throw Exception(At_SC_Phrase(func_part(call_phrase), f),
        "this function is not supported");
}

Value
Legacy_Function::call(Value arg, Frame& f)
{
    if (nargs_ == 1) {
        f[0] = arg;
        return call(f);
    }
    At_Arg cx(*this, f);
    auto list = arg.to<const List>(cx);
    list->assert_size(nargs_,cx);
    for (size_t i = 0; i < list->size(); ++i)
        f[i] = (*list)[i];
    return call(f);
}

Value
Legacy_Function::try_call(Value arg, Frame& f)
{
    if (nargs_ == 1) {
        f[0] = arg;
        return call(f);
    }
    auto list = arg.dycast<const List>();
    if (list && list->size() == nargs_) {
        for (size_t i = 0; i < list->size(); ++i)
            f[i] = (*list)[i];
        return call(f);
    } else {
        return missing;
    }
}

SC_Value
Legacy_Function::sc_call_expr(
    Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& f)
const
{
    auto f2 = SC_Frame::make(nslots_, f.sc_, nullptr, &f, call_phrase);
    if (nargs_ == 1)
        (*f2)[0] = sc_eval_op(f, arg);
    else if (auto list = dynamic_cast<List_Expr*>(&arg)) {
        if (list->size() != nargs_)
            throw Exception(At_SC_Phrase(arg.syntax_, f), stringify(
                "wrong number of arguments (got ",list->size(),
                ", expected ",nargs_,")"));
        for (size_t i = 0; i < list->size(); ++i)
            (*f2)[i] = sc_eval_op(f, *(*list)[i]);
    } else {
        auto scarg = sc_eval_op(f, arg);
        if (!scarg.type.is_vec())
            throw Exception(At_SC_Phrase(arg.syntax_, f),
                "function call argument is not a vector");
        if (scarg.type.count() != nargs_)
            throw Exception(At_SC_Phrase(arg.syntax_, f), stringify(
                "wrong number of arguments (got ",scarg.type.count(),
                ", expected ",nargs_,")"));
        for (unsigned i = 0; i < scarg.type.count(); ++i)
            (*f2)[i] = sc_vec_element(f, scarg, i);
    }
    return sc_call(*f2);
}

SC_Value
Legacy_Function::sc_call(SC_Frame& f) const
{
    throw Exception(At_SC_Frame(f), "this function is not supported");
}

Value
Closure::call(Value arg, Frame& f)
{
    f.nonlocals_ = &*nonlocals_;
    pattern_->exec(f.array_, arg, At_Arg(*this, f), f);
    return expr_->eval(f);
}

void
Closure::tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    f->nonlocals_ = &*nonlocals_;
    pattern_->exec(f->array_, arg, At_Arg(*this, *f), *f);
    f->next_op_ = &*expr_;
}

Value
Closure::try_call(Value arg, Frame& f)
{
    f.nonlocals_ = &*nonlocals_;
    if (!pattern_->try_exec(f.array_, arg, At_Arg(*this, f), f))
        return missing;
    return expr_->eval(f);
}

bool
Closure::try_tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    f->nonlocals_ = &*nonlocals_;
    if (!pattern_->try_exec(f->array_, arg, At_Arg(*this, *f), *f))
        return false;
    f->next_op_ = &*expr_;
    return true;
}

SC_Value
Closure::sc_call_expr(Operation& arg, Shared<const Phrase> cp, SC_Frame& f) const
{
    // create a frame to call this closure
    auto f2 = SC_Frame::make(nslots_, f.sc_, nullptr, &f, cp);
    f2->nonlocals_ = &*nonlocals_;
    // match pattern against argument, store formal parameters in frame
    pattern_->sc_exec(arg, f, *f2);
    // evaluation function body, return result.
    return sc_eval_op(*f2, *expr_);
}

void
Lambda::print(std::ostream& out) const
{
    out << "<lambda>";
}

slot_t
Piecewise_Function::maxslots(std::vector<Shared<Function>>& cases)
{
    slot_t result = 0;
    for (auto c : cases)
        result = std::max(result, c->nslots_);
    return result;
}

Value
Piecewise_Function::call(Value arg, Frame& f)
{
    for (auto c : cases_) {
        Value result = c->try_call(arg, f);
        if (!result.eq(missing))
            return result;
    }
    throw Exception(At_Arg(*this, f), stringify(
        arg," has no matching pattern"));
}

void
Piecewise_Function::tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    for (auto c : cases_) {
        if (c->try_tail_call(arg, f))
            return;
    }
    throw Exception(At_Arg(*this, *f), stringify(
        arg," has no matching pattern"));
}

Value
Piecewise_Function::try_call(Value arg, Frame& f)
{
    for (auto c : cases_) {
        Value result = c->try_call(arg, f);
        if (!result.eq(missing))
            return result;
    }
    return missing;
}

bool
Piecewise_Function::try_tail_call(Value arg, std::unique_ptr<Frame>& f)
{
    for (auto c : cases_) {
        if (c->try_tail_call(arg, f))
            return true;
    }
    return false;
}

SC_Value
Piecewise_Function::sc_call_expr(
    Operation& arg, Shared<const Phrase> call_phrase, SC_Frame& f)
const
{
    throw Exception(At_SC_Phrase(func_part(call_phrase), f),
        "piecewise function is not supported");
}

} // namespace curv
