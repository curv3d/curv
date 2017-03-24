// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_EVAL_H
#define CURV_EVAL_H

#include <curv/builtin.h>
#include <curv/frame.h>
#include <curv/meaning.h>
#include <curv/module.h>
#include <curv/script.h>
#include <curv/shared.h>
#include <curv/system.h>

namespace curv {

Shared<Module>
eval_module_script(const Script&, const Namespace&, System&, Frame* f = nullptr);

inline Shared<Module>
eval_module_script(const Script& scr, System& sys, Frame* f = nullptr)
{
    return eval_module_script(scr, sys.std_namespace(), sys, f);
}

} // namespace curv
#endif // header guard
