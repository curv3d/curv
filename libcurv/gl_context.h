// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_CONTEXT_H
#define LIBCURV_GL_CONTEXT_H

#include <libcurv/context.h>
#include <libcurv/gl_frame.h>

namespace curv {

struct At_GL_Frame : public Context
{
    GL_Frame& call_frame_;

    At_GL_Frame(GL_Frame& frame) : call_frame_(frame) {}

    virtual void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

/// Exception Context where we know the Phrase that contains the error.
struct At_GL_Phrase : public Context
{
    Shared<const Phrase> phrase_;
    GL_Frame& call_frame_;

    At_GL_Phrase(Shared<const Phrase> phrase, GL_Frame& frame);

    virtual void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

/// The exception context for the i'th argument in a function call.
struct At_GL_Arg : public Context
{
    size_t arg_index_;
    GL_Frame& call_frame_;

    At_GL_Arg(size_t i, GL_Frame& f) : arg_index_(i), call_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

void get_gl_frame_locations(const GL_Frame* f, std::list<Location>& locs);
Shared<const String> gl_frame_rewrite_message(
    const GL_Frame*, Shared<const String>);

} // namespace curv
#endif // header guard
