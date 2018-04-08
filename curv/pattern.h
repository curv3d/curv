// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_PATTERN_H
#define CURV_PATTERN_H

#include <curv/value.h>
#include <curv/frame.h>
#include <curv/gl_compiler.h>

namespace curv {

struct Environ;
struct Scope;
struct Phrase;
struct Context;

struct Pattern : public Shared_Base
{
    Shared<const Phrase> source_;

    Pattern(Shared<const Phrase> s) : Shared_Base(), source_(std::move(s)) {}

    virtual void analyse(Environ&) = 0;
    virtual void exec(Value* slots, Value, const Context&, Frame&) const = 0;
    virtual bool try_exec(Value* slots, Value, Frame&) const = 0;
    virtual void gl_exec(GL_Value, const Context&, GL_Frame&) const;
    virtual void gl_exec(Operation& expr, GL_Frame& caller, GL_Frame& callee) const;
};

Shared<Pattern> make_pattern(const Phrase&, Scope&, unsigned unitno);

} // namespace
#endif // header guard
