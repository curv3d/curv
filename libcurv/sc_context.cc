// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>
#include <libcurv/function.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>

namespace curv {

void
At_SC_Frame::get_locations(std::list<Func_Loc>& locs) const
{
    get_sc_frame_locations(&call_frame_, locs);
}
System& At_SC_Frame::system() const { return call_frame_.sc_.sstate_.system_; }
Frame* At_SC_Frame::frame() const { return nullptr; }

Shared<const String>
At_SC_Frame::rewrite_message(Shared<const String> msg) const
{
    return sc_frame_rewrite_message(&call_frame_, msg);
}

Shared<const Function> sc_frame_caller(const SC_Frame& f)
{
    if (auto pf = f.parent_frame_)
        return pf->func_;
    return nullptr;
}
void
get_sc_frame_locations(const SC_Frame* f, std::list<Func_Loc>& locs)
{
    for (; f != nullptr; f = f->parent_frame_) {
        if (f->call_phrase_ != nullptr)
            locs.emplace_back(sc_frame_caller(*f), f->call_phrase_->location());
        if (f->root_context_ != nullptr)
            f->root_context_->get_locations(locs);
    }
}

Shared<const String>
sc_frame_rewrite_message(const SC_Frame* f, Shared<const String> msg)
{
    for (; f != nullptr; f = f->parent_frame_) {
        if (f->root_context_ != nullptr)
            msg = f->root_context_->rewrite_message(msg);
    }
    msg = stringify("Shape Compiler: ", msg);
    return msg;
}

At_SC_Phrase::At_SC_Phrase(Shared<const Phrase> phrase, SC_Frame& frame)
: phrase_(move(phrase)), call_frame_(frame)
{}

void
At_SC_Phrase::get_locations(std::list<Func_Loc>& locs) const
{
    if (phrase_)
        locs.emplace_back(call_frame_.func_, phrase_->location());
    get_sc_frame_locations(&call_frame_, locs);
}
System& At_SC_Phrase::system() const { return call_frame_.sc_.sstate_.system_; }
Frame* At_SC_Phrase::frame() const { return nullptr; }
const Phrase& At_SC_Phrase::syntax() const { return *phrase_; }

Shared<const String>
At_SC_Phrase::rewrite_message(Shared<const String> msg) const
{
    return sc_frame_rewrite_message(&call_frame_, msg);
}

void At_SC_Tuple_Arg::get_locations(std::list<Func_Loc>& locs) const
{
    get_sc_frame_locations(&call_frame_, locs);
}
System& At_SC_Tuple_Arg::system() const { return call_frame_.sc_.sstate_.system_; }
Frame* At_SC_Tuple_Arg::frame() const { return nullptr; }

Shared<const String>
At_SC_Tuple_Arg::rewrite_message(Shared<const String> msg) const
{
    return sc_frame_rewrite_message(&call_frame_,
        stringify("argument[",tuple_index_,"]: ", msg));
}

void
At_SC_Arg_Expr::get_locations(std::list<Func_Loc>& locs) const
{
    locs.emplace_back(sc_frame_caller(parent_frame_),
        arg_part(call_phrase_)->location());
    get_sc_frame_locations(&parent_frame_, locs);
}
Shared<const String>
At_SC_Arg_Expr::rewrite_message(Shared<const String> msg) const
{
    if (func_.name_.empty())
        msg = stringify("function argument: ",msg);
    else
       msg = stringify("argument #",func_.argpos_+1," of ",func_.name_,": ",msg);
    return sc_frame_rewrite_message(&parent_frame_, msg);
}
System&
At_SC_Arg_Expr::system() const
{
    return parent_frame_.sc_.sstate_.system_;
}
Frame*
At_SC_Arg_Expr::frame() const
{
    return nullptr;
}
const Phrase&
At_SC_Arg_Expr::syntax() const
{
    return *arg_part(call_phrase_);
}

} // namespace curv
