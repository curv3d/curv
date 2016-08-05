// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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
