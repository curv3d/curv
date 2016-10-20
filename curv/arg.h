// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ARG_H
#define CURV_ARG_H

#include <curv/list.h>
#include <curv/string.h>
#include <curv/phrase.h>

namespace curv {

List& arg_to_list(Value, const Phrase&);
String& arg_to_string(Value, const Phrase&);
int arg_to_int(Value, int, int, const Phrase&);

/// Get the source code for the i'th function argument from argsource.
/// Used to construct an argument for `arg_to_*()` or `throw Phrase_Error()`.
///
/// TODO: Consider the performance cost of this function, in the case where
/// it is used with `arg_to_*()`.
const Phrase& get_arg(const Phrase& argsource, size_t i);

} // namespace curv
#endif // header guard
