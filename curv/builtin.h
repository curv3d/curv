// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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
