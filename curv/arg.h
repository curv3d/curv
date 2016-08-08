// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ARG_H
#define CURV_ARG_H

#include <curv/list.h>
#include <curv/phrase.h>

namespace curv {

List& arg_to_list(Value, const char*);

List& arg_to_list(Value, const Phrase&);
int arg_to_int(Value, int, int, const Phrase&);

} // namespace curv
#endif // header guard
