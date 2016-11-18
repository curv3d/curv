// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <fstream>
#include <curv/analyzer.h>
#include <curv/builtin.h>
#include <curv/context.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/system.h>

auto curv::eval_script(
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

auto curv::eval_file(const String& path, System& sys, Frame* f)
-> Shared<Module>
{
    // TODO: Cache multiple references to the same file.
    // TODO: Pluggable file system abstraction, for unit testing and
    // abstracting the behaviour of `file` (would also support caching).
    // TODO: More precise error message when open fails.
    std::ifstream t;
    t.open(path.c_str());
    if (t.fail())
        throw Exception(At_Frame(f),
            stringify("can't open file ", path.c_str()));
    String_Builder buffer;
    buffer << t.rdbuf();
    auto script = aux::make_shared<String_Script>(
        Shared<const String>(&path), buffer.get_string());
    return eval_script(*script, sys.std_namespace(), sys, f);
}
