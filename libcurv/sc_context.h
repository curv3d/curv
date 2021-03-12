// Copyright 2016-2020 Doug Moen
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

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
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

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

// This is part of v1 of the API for builtin SC function call.
// It is used by Tuple_Function::sc_tuple_call() in builtin.cc.
// See At_SC_Arg_Expr for the v2 version.
struct At_SC_Tuple_Arg : public Context
{
    size_t tuple_index_;
    SC_Frame& call_frame_;  // frame for THIS function call

    At_SC_Tuple_Arg(size_t i, SC_Frame& f) : tuple_index_(i), call_frame_(f) {}

    void get_locations(std::list<Func_Loc>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

// This is part of v2 of the API for builtin SC function call.
// It is used by Function::sc_call_expr() in builtin.cc.
// It closely models the semantics of At_Arg from context.h.
// See At_SC_Tuple_Arg for the v1 version.
struct At_SC_Arg_Expr : public At_Syntax
{
    At_SC_Arg_Expr(
        const Function& fn,
        Shared<const Phrase> callphrase,
        SC_Frame& parentframe)
    :
        func_(fn),
        call_phrase_(callphrase),
        parent_frame_(parentframe)
    {}

    const Function& func_;
    Shared<const Phrase> call_phrase_;
    SC_Frame& parent_frame_;    // The CALLER's frame. This call has no frame.

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

void get_sc_frame_locations(const SC_Frame* f, std::list<Func_Loc>& locs);
Shared<const String> sc_frame_rewrite_message(
    const SC_Frame*, Shared<const String>);

} // namespace curv
#endif // header guard
