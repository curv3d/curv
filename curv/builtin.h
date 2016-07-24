// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_BUILTIN_H
#define CURV_BUILTIN_H

#include <curv/atom.h>
#include <curv/value.h>

namespace curv {

using Namespace = Atom_Map<Value>;

/// The Curv language builtin bindings.
extern const Namespace builtin_namespace;

} // namespace curv
#endif // header guard
