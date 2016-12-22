// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/analyzer.h>
#include <curv/gl_context.h>

using namespace curv;

void
At_GL_Frame::get_locations(std::list<Location>& locs) const
{
    get_gl_frame_locations(frame_, locs);
}

void
curv::get_gl_frame_locations(const GL_Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame)
        if (f->call_phrase != nullptr)
            locs.push_back(f->call_phrase->location());
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
    assert(eval_frame_.call_phrase != nullptr);

    const Phrase& arg = eval_frame_.call_phrase->at(arg_index_);

    locs.push_back(arg.location());
    // We only dump the stack starting at the parent call frame,
    // for cosmetic reasons. It looks stupid to underline one of the
    // arguments in a function call, and on the next line,
    // underline the same entire function call.
    get_gl_frame_locations(eval_frame_.parent_frame, locs);
}
