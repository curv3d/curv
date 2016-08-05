// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/scanner.h>
#include <curv/parse.h>
#include <curv/analyzer.h>
#include <curv/builtin.h>
#include <curv/eval.h>

auto curv::eval_script(const Script& script, const Namespace& names)
-> Shared<Module>
{
    Scanner scanner{script};
    auto phrase = parse_script(scanner);
    Environ env{names};
    auto expr = analyze_expr(*phrase, env);
    auto value = eval(*expr);
    assert(value.is_ref());
    Ref_Value& ref{value.get_ref_unsafe()};
    Module& module{dynamic_cast<Module&>(ref)};
    return Shared<Module>(&module);
}
