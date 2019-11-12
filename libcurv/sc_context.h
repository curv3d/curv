// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_CONTEXT_H
#define LIBCURV_SC_CONTEXT_H

#include <libcurv/context.h>
#include <libcurv/sc_frame.h>

namespace curv {

struct At_SC_Frame : public Context
{
    SC_Frame& call_frame_;

    At_SC_Frame(SC_Frame& frame) : call_frame_(frame) {}

    virtual void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

/// Exception Context where we know the Phrase that contains the error.
struct At_SC_Phrase : public At_Syntax
{
    Shared<const Phrase> phrase_;
    SC_Frame& call_frame_;

    At_SC_Phrase(Shared<const Phrase> phrase, SC_Frame& frame);

    virtual void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

/// The exception context for the i'th argument in a function call.
struct At_SC_Arg : public Context
{
    size_t arg_index_;
    SC_Frame& call_frame_;

    At_SC_Arg(size_t i, SC_Frame& f) : arg_index_(i), call_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

void get_sc_frame_locations(const SC_Frame* f, std::list<Location>& locs);
Shared<const String> sc_frame_rewrite_message(
    const SC_Frame*, Shared<const String>);

} // namespace curv
#endif // header guard
