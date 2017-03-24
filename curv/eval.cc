// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/analyzer.h>
#include <curv/builtin.h>
#include <curv/eval.h>
#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/system.h>

namespace curv {

Shared<Module>
eval_module_script(
    const Script& script, const Namespace& names,
    System& sys, Frame* f)
{
    Scanner scanner{script, f};
    auto phrase = parse_program(scanner);
    Builtin_Environ env{names, f};
    auto expr = phrase->analyze_module(env);
    auto value = expr->eval_module(sys, f);
    return value;
}

void
Eval::compile(const Namespace* names, Frame* parent_frame)
{
    if (names == nullptr)
        names_ = &system_.std_namespace();
    else
        names_ = names;
    parent_frame_ = parent_frame;

    Scanner scanner{script_, parent_frame};
    phrase_ = parse_program(scanner);

    Builtin_Environ env{*names_, parent_frame};
    meaning_ = phrase_->analyze(env);

    frame_ = {Frame::make(env.frame_maxslots_,
        system_, parent_frame, nullptr, nullptr)};
}

Value
Eval::eval()
{
    auto expr = meaning_->to_operation(parent_frame_);
    return expr->eval(*frame_);
}

} // namespace curv
