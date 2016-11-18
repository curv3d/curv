// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_EVAL_H
#define CURV_EVAL_H

#include <curv/frame.h>
#include <curv/meaning.h>
#include <curv/shared.h>
#include <curv/module.h>
#include <curv/script.h>
#include <curv/builtin.h>

namespace curv {

inline Value eval(const Operation& expr, Frame& f) { return expr.eval(f); }

Shared<Module> eval_script(const Script&, const Namespace& names, System&, Frame* f = nullptr);

Shared<Module> eval_file(const String& filename, System&, Frame* f = nullptr);

} // namespace curv
#endif // header guard
