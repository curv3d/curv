// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_ANALYZER_H
#define CURV_ANALYZER_H

#include <curv/meaning.h>
#include <curv/eval.h>

namespace curv {

struct AContext
{
    const Namespace& names;
    AContext(const Namespace& n) : names(n) {}
};

inline Shared<Meaning>
analyze(Phrase& ph, AContext ctx)
{
    return ph.analyze(ctx);
}

Shared<Expression> analyze_expr(Phrase& ph, AContext ctx);

} // namespace
#endif // header guard
