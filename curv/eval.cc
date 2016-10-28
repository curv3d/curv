// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <fstream>
#include <curv/analyzer.h>
#include <curv/builtin.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/parse.h>
#include <curv/scanner.h>

auto curv::eval_script(const Script& script, const Namespace& names,
    Frame* f)
-> Shared<Module>
{
    Scanner scanner{script, f};
    auto phrase = parse_script(scanner);
    Builtin_Environ env{names};
    auto expr = phrase->analyze_module(env);
    auto value = expr->eval_module(f);
    return value;
}

auto curv::eval_file(const String& path, Frame* f)
-> Shared<Module>
{
    // TODO: Cache multiple references to the same file.
    // TODO: Pluggable file system abstraction, for unit testing and
    // abstracting the behaviour of `file` (would also support caching).
    // TODO: More precise error message when open fails.
    // TODO: The builtin_namespace used by `file` is a pluggable parameter
    // at compile time.
    std::ifstream t;
    t.open(path.c_str());
    if (t.fail())
        throw Exception(At_Frame(f),
            stringify("can't open file ", path.c_str()));
    String_Builder buffer;
    buffer << t.rdbuf();
    auto script = aux::make_shared<String_Script>(
        Shared<const String>(&path), buffer.get_string());
    return eval_script(*script, builtin_namespace, f);
}
