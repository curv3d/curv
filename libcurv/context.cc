// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/scanner.h>

namespace curv {

void
Context::get_locations(std::list<Location>&) const
{
}

Shared<const String>
Context::rewrite_message(Shared<const String> msg) const
{
    return msg;
}

void
At_Frame::get_locations(std::list<Location>& locs) const
{
    get_frame_locations(frame_, locs);
}

void
get_frame_locations(const Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame_)
        if (f->call_phrase_ != nullptr)
            locs.push_back(f->call_phrase_->location());
}

At_Token::At_Token(Token tok, const Scanner& scanner)
:
    loc_{*scanner.source_, tok},
    eval_frame_(scanner.eval_frame_)
{
}

At_Token::At_Token(Token tok, const Phrase& phrase, Environ& env)
:
    loc_{phrase.location().source(), tok},
    eval_frame_{env.eval_frame_}
{
}

At_Token::At_Token(Location loc, Environ& env)
:
    loc_(std::move(loc)),
    eval_frame_{env.eval_frame_}
{
}

void
At_Token::get_locations(std::list<Location>& locs) const
{
    locs.push_back(loc_);
    get_frame_locations(eval_frame_, locs);
}

At_Phrase::At_Phrase(const Phrase& phrase, Frame* frame)
: phrase_(phrase), frame_(frame)
{}

At_Phrase::At_Phrase(const Phrase& phrase, Environ& env)
: phrase_(phrase), frame_(env.eval_frame_)
{}

void
At_Phrase::get_locations(std::list<Location>& locs) const
{
    locs.push_back(phrase_.location());
    get_frame_locations(frame_, locs);
}

void
At_Arg::get_locations(std::list<Location>& locs) const
{
    if (eval_frame_.call_phrase_ != nullptr) {
        auto arg = eval_frame_.call_phrase_->arg_;
        locs.push_back(arg->location());
        // We only dump the stack starting at the parent call frame,
        // for cosmetic reasons. It looks stupid to underline one of the
        // arguments in a function call, and on the next line,
        // underline the same entire function call.
        get_frame_locations(eval_frame_.parent_frame_, locs);
    } else {
        get_frame_locations(&eval_frame_, locs);
    }
}

Shared<const String>
At_Arg::rewrite_message(Shared<const String> msg) const
{
    if (fun_.name_.empty())
        return stringify("function argument: ",msg);
    return stringify("argument #",fun_.argpos_+1," of ",fun_.name_,": ",msg);
}

void
At_Metacall::get_locations(std::list<Location>& locs) const
{
    locs.push_back(arg_.location());
    get_frame_locations(frame_, locs);
}

Shared<const String>
At_Metacall::rewrite_message(Shared<const String> msg) const
{
    return stringify("argument #",argpos_+1," of ",name_,": ",msg);
}

At_Field::At_Field(const char* fieldname, const Context& parent)
: fieldname_(fieldname), parent_(parent)
{}

void
At_Field::get_locations(std::list<Location>& locs) const
{
    parent_.get_locations(locs);
}

Shared<const String>
At_Field::rewrite_message(Shared<const String> msg) const
{
    return stringify("at field .",fieldname_,": ",parent_.rewrite_message(msg));
}

At_Index::At_Index(size_t index, const Context& parent)
: index_(index), parent_(parent)
{}

void
At_Index::get_locations(std::list<Location>& locs) const
{
    parent_.get_locations(locs);
}

Shared<const String>
At_Index::rewrite_message(Shared<const String> msg) const
{
    return stringify("at index [",index_,"]: ",parent_.rewrite_message(msg));
}

} // namespace curv
