// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_ANALYZER_H
#define CURV_ANALYZER_H

#include <curv/meaning.h>
#include <curv/builtin.h>

namespace curv {

struct Environ
{
    const Namespace& names;
    Environ(const Namespace& n) : names(n) {}
};

inline Shared<Meaning>
analyze(Phrase& ph, const Environ& env)
{
    return ph.analyze(env);
}

Shared<Expression> analyze_expr(Phrase& ph, const Environ& env);

} // namespace
#endif // header guard
