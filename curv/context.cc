// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/context.h>
#include <curv/analyzer.h>

using namespace curv;

void
curv::Context::get_locations(std::list<Location>&) const
{
}

void
curv::At_Frame::get_locations(std::list<Location>& locs) const
{
    get_frame_locations(frame_, locs);
}

void
curv::get_frame_locations(const Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame)
        if (f->call_phrase != nullptr)
            locs.push_back(f->call_phrase->location());
}

void
curv::At_Location::get_locations(std::list<Location>& locs) const
{
    locs.push_back(loc_);
    get_frame_locations(eval_frame_, locs);
}

curv::At_Phrase::At_Phrase(const Phrase& phrase, Frame* frame)
: phrase_(phrase), frame_(frame)
{}

curv::At_Phrase::At_Phrase(const Phrase& phrase, Environ& env)
: phrase_(phrase), frame_(env.eval_frame_)
{}

void
curv::At_Phrase::get_locations(std::list<Location>& locs) const
{
    locs.push_back(phrase_.location());
    get_frame_locations(frame_, locs);
}
