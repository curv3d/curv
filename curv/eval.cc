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

auto eval_script(
    const Script& script, const Namespace& names,
    System& sys, Frame* f)
-> Shared<Module>
{
    Scanner scanner{script, f};
    auto phrase = parse_script(scanner);
    Builtin_Environ env{names, f};
    auto expr = phrase->analyze_module(env);
    auto value = expr->eval_module(sys, f);
    return value;
}

} // namespace curv
