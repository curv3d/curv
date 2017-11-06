// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_PATTERN_H
#define CURV_PATTERN_H

#include <curv/value.h>
#include <curv/frame.h>

namespace curv {

struct Environ;
struct Scope;
struct Phrase;
struct Context;

struct Pattern : public Shared_Base
{
    Pattern() : Shared_Base() {}

    virtual void analyze(Environ&) = 0;
    virtual void exec(Value* slots, Value, const Context&, Frame&) = 0;
};

Shared<Pattern> make_pattern(const Phrase&, Scope&, unsigned unitno);

} // namespace
#endif // header guard
