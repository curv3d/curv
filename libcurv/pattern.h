// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PATTERN_H
#define LIBCURV_PATTERN_H

#include <libcurv/phrase.h>
#include <libcurv/sc_frame.h>
#include <libcurv/sstate.h>
#include <libcurv/value.h>

namespace curv {

struct Closure;
struct Context;
struct Environ;
struct Frame;
struct Operation;
struct Record;
struct Scope;

struct Pattern : public Shared_Base
{
    Shared<const Phrase> syntax_;

    Pattern(Shared<const Phrase> s)
    :
        Shared_Base(),
        syntax_(std::move(s))
    {}

    virtual void add_to_scope(Scope&, unsigned unitno) = 0;
    virtual void analyse(Environ&) = 0;
    virtual void exec(Value* slots, Value, const Context&, Frame&) const = 0;
    virtual bool try_exec(Value* slots, Value, const Context&, Frame&) const = 0;
    virtual void sc_exec(SC_Value, const Context&, SC_Frame&) const;
    virtual void sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee) const;
};

Shared<Pattern> make_pattern(const Phrase&, Environ&);

Shared<Record> record_pattern_default_value(const Pattern&, Frame&);

void record_pattern_each_parameter(
    const Closure&, Source_State&,
    std::function<void(Symbol_Ref, Value, Value, Shared<const Phrase>)>);

} // namespace
#endif // header guard
