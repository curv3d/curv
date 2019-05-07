// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PATTERN_H
#define LIBCURV_PATTERN_H

#include <libcurv/value.h>
#include <libcurv/frame.h>
#include <libcurv/sc_frame.h>

namespace curv {

struct Environ;
struct Scope;
struct Phrase;
struct Context;
struct Record;
struct Closure;
struct Operation;

struct Pattern : public Shared_Base
{
    Shared<const Phrase> syntax_;

    Pattern(Shared<const Phrase> s)
    :
        Shared_Base(),
        syntax_(std::move(s))
    {}

    virtual void analyse(Environ&) = 0;
    virtual void exec(Value* slots, Value, const Context&, Frame&) const = 0;
    virtual bool try_exec(Value* slots, Value, const Context&, Frame&) const = 0;
    virtual void sc_exec(SC_Value, const Context&, SC_Frame&) const;
    virtual void sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee) const;
};

Shared<Pattern> make_pattern(const Phrase&, bool mut, Scope&, unsigned unitno);

Shared<Record> record_pattern_default_value(const Pattern&, Frame&);

void record_pattern_each_parameter(
    Closure&, System&, std::function<void(Symbol_Ref, Value, Value)>);

} // namespace
#endif // header guard
