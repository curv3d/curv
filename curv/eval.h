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

struct Eval
{
    const Script& script_;
    System& system_;
    const Namespace* names_ = nullptr;
    Frame *parent_frame_ = nullptr;
    Shared<const Phrase> phrase_ = nullptr;
    Shared<Meaning> meaning_ = nullptr;
    std::unique_ptr<Frame> frame_ = nullptr;

    Eval(
        const Script& script,
        System& system)
    :
        script_(script),
        system_(system)
    {}

    void compile(
        const Namespace* names = nullptr,
        Frame *parent_frame = nullptr);

    Value eval();
};

} // namespace curv
#endif // header guard
