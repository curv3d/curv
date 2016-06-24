// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EVAL_H
#define CURV_EVAL_H

#include <string>
#include <map>
#include <curv/phrase.h>
#include <curv/value.h>
#include <curv/meaning.h>

namespace curv {

using Namespace = std::map<std::string, Value>;

// the Curv builtin bindings
extern const Namespace builtin_namespace;

inline Value eval(const Expression& e) { return e.eval(); }

} // namespace curv
#endif // header guard
