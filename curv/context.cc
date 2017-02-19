// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/analyzer.h>
#include <curv/context.h>
#include <curv/scanner.h>

namespace curv {

void
Context::get_locations(std::list<Location>&) const
{
}

void
At_Frame::get_locations(std::list<Location>& locs) const
{
    get_frame_locations(frame_, locs);
}

void
get_frame_locations(const Frame* f, std::list<Location>& locs)
{
    for (; f != nullptr; f = f->parent_frame)
        if (f->call_phrase != nullptr)
            locs.push_back(f->call_phrase->location());
}

At_Token::At_Token(Token tok, const Scanner& scanner)
:
    loc_{scanner.script_, tok},
    eval_frame_(scanner.eval_frame_)
{
}

At_Token::At_Token(Token tok, const Phrase& phrase, Environ& env)
:
    loc_{phrase.location().script(), tok},
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

} // namespace curv
