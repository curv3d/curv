// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>
#include <libcurv/gl_context.h>

namespace curv {

void
At_GL_Frame::get_locations(std::list<Location>& locs) const
{
    get_gl_frame_locations(frame_, locs);
}

Shared<const String>
At_GL_Frame::rewrite_message(Shared<const String> msg) const
{
    return gl_frame_rewrite_message(frame_, msg);
}

void
get_gl_frame_locations(const GL_Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame_) {
        if (f->call_phrase_ != nullptr)
            locs.push_back(f->call_phrase_->location());
        if (f->root_context_ != nullptr)
            f->root_context_->get_locations(locs);
    }
}

Shared<const String>
gl_frame_rewrite_message(const GL_Frame* f, Shared<const String> msg)
{
    for (; f != nullptr; f = f->parent_frame_) {
        if (f->root_context_ != nullptr)
            msg = f->root_context_->rewrite_message(msg);
    }
    return msg;
}

At_GL_Phrase::At_GL_Phrase(Shared<const Phrase> phrase, GL_Frame* frame)
: phrase_(std::move(phrase)), frame_(frame)
{}

void
At_GL_Phrase::get_locations(std::list<Location>& locs) const
{
    if (phrase_)
        locs.push_back(phrase_->location());
    get_gl_frame_locations(frame_, locs);
}

Shared<const String>
At_GL_Phrase::rewrite_message(Shared<const String> msg) const
{
    return gl_frame_rewrite_message(frame_, msg);
}

void At_GL_Arg::get_locations(std::list<Location>& locs) const
{
    get_gl_frame_locations(&eval_frame_, locs);
}

Shared<const String>
At_GL_Arg::rewrite_message(Shared<const String> msg) const
{
    return gl_frame_rewrite_message(&eval_frame_,
        stringify("argument[",arg_index_,"]: ", msg));
}

} // namespace curv
