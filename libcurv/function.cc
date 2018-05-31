// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/gl_context.h>

namespace curv {

const char Function::name[] = "function";

void
Function::print(std::ostream& out) const
{
    if (name_.empty())
        out << "<function>";
    else
        out << name_;
    for (int i = 0; i < argpos_; ++i)
        out << " _";
}

GL_Value
Function::gl_call_expr(Operation& arg, const Call_Phrase* call_phrase, GL_Frame& f)
const
{
    throw Exception(At_GL_Phrase(call_phrase->function_, &f),
        "this function does not support the Geometry Compiler");
}

Value
Legacy_Function::call(Value arg, Frame& f)
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
        throw Exception(At_Phrase(*f.call_phrase_->arg_, &f),
            stringify("function call argument is not a list of length ",nargs_));
    }
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

GL_Value
Legacy_Function::gl_call_expr(
    Operation& arg, const Call_Phrase* call_phrase, GL_Frame& f)
const
{
    auto f2 = GL_Frame::make(nslots_, f.gl, nullptr, &f, call_phrase);
    if (nargs_ == 1)
        (*f2)[0] = arg.gl_eval(f);
    else if (auto list = dynamic_cast<List_Expr*>(&arg)) {
        if (list->size() != nargs_)
            throw Exception(At_GL_Phrase(arg.source_, &f), stringify(
                "wrong number of arguments (got ",list->size(),
                ", expected ",nargs_,")"));
        for (size_t i = 0; i < list->size(); ++i)
            (*f2)[i] = (*list)[i]->gl_eval(f);
    } else {
        auto glarg = arg.gl_eval(f);
        if (!gl_type_is_vec(glarg.type))
            throw Exception(At_GL_Phrase(arg.source_, &f),
                "function call argument is not a vector");
        if (gl_type_count(glarg.type) != nargs_)
            throw Exception(At_GL_Phrase(arg.source_, &f), stringify(
                "wrong number of arguments (got ",gl_type_count(glarg.type),
                ", expected ",nargs_,")"));
        for (unsigned i = 0; i < gl_type_count(glarg.type); ++i)
            (*f2)[i] = gl_vec_element(f, glarg, i);
    }
    return gl_call(*f2);
}

GL_Value
Legacy_Function::gl_call(GL_Frame& f) const
{
    throw Exception(At_GL_Frame(&f),
        "this function does not support the Geometry Compiler");
}

Value
Closure::call(Value arg, Frame& f)
{
    f.nonlocals_ = &*nonlocals_;
    if (f.call_phrase_ != nullptr)
        pattern_->exec(f.array_, arg, At_Phrase(*f.call_phrase_->arg_,&f), f);
    else
        pattern_->exec(f.array_, arg, {}, f);
    return expr_->eval(f);
}

Value
Closure::try_call(Value arg, Frame& f)
{
    f.nonlocals_ = &*nonlocals_;
    if (!pattern_->try_exec(f.array_, arg, f))
        return missing;
    return expr_->eval(f);
}

GL_Value
Closure::gl_call_expr(Operation& arg, const Call_Phrase* cp, GL_Frame& f) const
{
    // create a frame to call this closure
    auto f2 = GL_Frame::make(nslots_, f.gl, nullptr, &f, cp);
    f2->nonlocals_ = &*nonlocals_;
    // match pattern against argument, store formal parameters in frame
    pattern_->gl_exec(arg, f, *f2);
    // evaluation function body, return result.
    return expr_->gl_eval(*f2);
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
Piecewise_Function::call(Value val, Frame& f)
{
    for (auto c : cases_) {
        Value result = c->try_call(val, f);
        if (result != missing)
            return result;
    }
    throw Exception(At_Phrase(*f.call_phrase_->arg_, &f), stringify(
        "argument ",val," has no matching pattern in switch"));
}
Value
Piecewise_Function::try_call(Value val, Frame& f)
{
    for (auto c : cases_) {
        Value result = c->try_call(val, f);
        if (result != missing)
            return result;
    }
    return missing;
}

} // namespace curv
