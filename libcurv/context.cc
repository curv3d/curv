// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/scanner.h>
#include <libcurv/exception.h>

namespace curv {

Shared<const String>
Context::rewrite_message(Shared<const String> msg) const
{
    return msg;
}

void At_System::get_locations(std::list<Location>& locs) const { }
System& At_System::system() const { return system_; }
Frame* At_System::frame() const { return nullptr; }

void
At_Frame::get_locations(std::list<Location>& locs) const
{
    get_frame_locations(&call_frame_, locs);
}
System& At_Frame::system() const { return call_frame_.system_; }
Frame* At_Frame::frame() const { return &call_frame_; }
const Phrase& At_Frame::syntax() const
{
    // A function call frame should always have a non-null call_phrase_.
    if (call_frame_.call_phrase_ == nullptr)
        throw Exception{*this,
            "Internal error: At_Frame::syntax(): call_phrase_ is null"};
    return *call_frame_.call_phrase_;
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
    system_{scanner.system_},
    file_frame_{scanner.file_frame_}
{
}

At_Token::At_Token(Token tok, const Phrase& phrase, Environ& env)
:
    loc_{phrase.location().source(), tok},
    system_{env.analyser_.system_},
    file_frame_{env.file_frame_}
{
}

At_Token::At_Token(Location loc, Environ& env)
:
    loc_{std::move(loc)},
    system_{env.analyser_.system_},
    file_frame_{env.file_frame_}
{
}

At_Token::At_Token(Location loc, System& sys, Frame* f)
:
    loc_{std::move(loc)},
    system_{sys},
    file_frame_{f}
{
}

void
At_Token::get_locations(std::list<Location>& locs) const
{
    locs.push_back(loc_);
    get_frame_locations(file_frame_, locs);
}
System& At_Token::system() const { return system_; }
Frame* At_Token::frame() const { return file_frame_; }

At_Phrase::At_Phrase(const Phrase& phrase, Frame& call_frame)
: phrase_(phrase), system_(call_frame.system_), frame_(&call_frame)
{}

At_Phrase::At_Phrase(const Phrase& phrase, System& sys, Frame* frame)
: phrase_(phrase), system_(sys), frame_(frame)
{}

At_Phrase::At_Phrase(const Phrase& phrase, Scanner& scanner)
: phrase_(phrase), system_(scanner.system_), frame_(scanner.file_frame_)
{}

At_Phrase::At_Phrase(const Phrase& phrase, Environ& env)
: phrase_(phrase), system_(env.analyser_.system_), frame_(env.file_frame_)
{}

void
At_Phrase::get_locations(std::list<Location>& locs) const
{
    locs.push_back(phrase_.location());
    get_frame_locations(frame_, locs);
}
System& At_Phrase::system() const { return system_; }
Frame* At_Phrase::frame() const { return frame_; }
const Phrase& At_Phrase::syntax() const { return phrase_; }

void
At_Arg::get_locations(std::list<Location>& locs) const
{
    if (call_frame_.call_phrase_ != nullptr) {
        auto arg = arg_part(call_frame_.call_phrase_);
        locs.push_back(arg->location());
        // We only dump the stack starting at the parent call frame,
        // for cosmetic reasons. It looks stupid to underline one of the
        // arguments in a function call, and on the next line,
        // underline the same entire function call.
        get_frame_locations(call_frame_.parent_frame_, locs);
    } else {
        get_frame_locations(&call_frame_, locs);
    }
}
System& At_Arg::system() const { return call_frame_.system_; }
Frame* At_Arg::frame() const { return &call_frame_; }
const Phrase& At_Arg::syntax() const
{
    // A function call frame should always have a non-null call_phrase_.
    if (call_frame_.call_phrase_ == nullptr)
        throw Exception{At_Frame(call_frame_),
            "Internal error: At_Arg::syntax(): call_phrase_ is null"};
    return *arg_part(call_frame_.call_phrase_);
}

Shared<const String>
At_Arg::rewrite_message(Shared<const String> msg) const
{
    if (func_.name_.empty())
        return stringify("function argument: ",msg);
    return stringify("argument #",func_.argpos_+1," of ",func_.name_,": ",msg);
}

void
At_Metacall::get_locations(std::list<Location>& locs) const
{
    locs.push_back(arg_.location());
    get_frame_locations(&parent_frame_, locs);
}
System& At_Metacall::system() const { return parent_frame_.system_; }
Frame* At_Metacall::frame() const { return &parent_frame_; }
const Phrase& At_Metacall::syntax() const { return arg_; }

Shared<const String>
At_Metacall::rewrite_message(Shared<const String> msg) const
{
    return stringify("argument #",argpos_+1," of ",name_,": ",msg);
}

void
At_Metacall_With_Call_Frame::get_locations(std::list<Location>& locs) const
{
    auto arg = arg_part(call_frame_.call_phrase_);
    locs.push_back(arg->location());
    get_frame_locations(call_frame_.parent_frame_, locs);
}
System& At_Metacall_With_Call_Frame::system() const
{
    return call_frame_.system_;
}
Frame* At_Metacall_With_Call_Frame::frame() const { return &call_frame_; }
const Phrase& At_Metacall_With_Call_Frame::syntax() const
{
    return *arg_part(call_frame_.call_phrase_);
}
Shared<const String>
At_Metacall_With_Call_Frame::rewrite_message(Shared<const String> msg) const
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
System& At_Field::system() const { return parent_.system(); }
Frame* At_Field::frame() const { return parent_.frame(); }

Shared<const String>
At_Field::rewrite_message(Shared<const String> msg) const
{
    return stringify("at field .",Symbol_Ref{fieldname_},": ",parent_.rewrite_message(msg));
}

At_Index::At_Index(size_t index, const Context& parent)
: index_(index), parent_(parent)
{}

void
At_Index::get_locations(std::list<Location>& locs) const
{
    parent_.get_locations(locs);
}
System& At_Index::system() const { return parent_.system(); }
Frame* At_Index::frame() const { return parent_.frame(); }

Shared<const String>
At_Index::rewrite_message(Shared<const String> msg) const
{
    return stringify("at index [",index_,"]: ",parent_.rewrite_message(msg));
}

} // namespace curv
