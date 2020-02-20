// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/phrase.h>

namespace curv {

Shared<const Phrase>
nub_phrase(Shared<const Phrase> ph)
{
    for (;;) {
        if (auto pr = cast<const Program_Phrase>(ph)) {
            ph = pr->body_;
            continue;
        }
        if (auto let = cast<const Let_Phrase>(ph)) {
            ph = let->body_;
            continue;
        }
        if (auto where = cast<const Where_Phrase>(ph)) {
            ph = where->left_;
            continue;
        }
        if (auto p = cast<const Paren_Phrase>(ph)) {
            if (isa<Empty_Phrase>(p->body_))
                break;
            if (isa<Comma_Phrase>(p->body_))
                break;
            ph = p->body_;
            continue;
        }
        break;
    }
    return ph;
}

Shared<const Phrase>
func_part(Shared<const Phrase> ph)
{
    auto cp = cast<const Call_Phrase>(ph);
    if (cp != nullptr)
        return cp->function_;
    else
        return ph;
}

Shared<const Phrase>
arg_part(Shared<const Phrase> ph)
{
    auto cp = cast<const Call_Phrase>(ph);
    if (cp != nullptr)
        return cp->arg_;
    else
        return ph;
}

} // namespace curv
