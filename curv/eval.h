// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_EVAL_H
#define CURV_EVAL_H

#include <curv/value.h>
#include <curv/meaning.h>
#include <curv/shared.h>
#include <curv/module.h>
#include <curv/script.h>
#include <curv/builtin.h>

namespace curv {

/// a Frame is an evaluation context.
/// TODO: Future design of Frame:
/// * Value* nonlocal;
/// * Value local[0]; // tail array
struct Frame
{
    Module& module_;
    Frame(Module& m) : module_(m) {}
};

inline Value eval(const Expression& e, Frame& f) { return e.eval(f); }

Shared<Module> eval_script(const Script&, const Namespace& names);

} // namespace curv
#endif // header guard
