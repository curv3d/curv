// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/analyzer.h>
#include <curv/gl_context.h>

namespace curv {

void
At_GL_Frame::get_locations(std::list<Location>& locs) const
{
    get_gl_frame_locations(frame_, locs);
}

void
get_gl_frame_locations(const GL_Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame_) {
        if (f->call_phrase_ != nullptr)
            locs.push_back(f->call_phrase_->location());
        if (f->root_context != nullptr)
            f->root_context->get_locations(locs);
    }
}

At_GL_Phrase::At_GL_Phrase(const Phrase& phrase, GL_Frame* frame)
: phrase_(phrase), frame_(frame)
{}

void
At_GL_Phrase::get_locations(std::list<Location>& locs) const
{
    locs.push_back(phrase_.location());
    get_gl_frame_locations(frame_, locs);
}

void At_GL_Arg::get_locations(std::list<Location>& locs) const
{
    get_gl_frame_locations(&eval_frame_, locs);
}

Shared<const String>
At_GL_Arg::rewrite_message(Shared<const String> msg) const
{
    return stringify("argument[",arg_index_,"]: ", msg);
}

} // namespace curv
