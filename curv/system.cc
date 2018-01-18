// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/context.h>
#include <curv/program.h>
#include <curv/file.h>
#include <curv/system.h>

namespace curv {

System_Impl::System_Impl(std::ostream& console)
:
    console_(console)
{
    std_namespace_ = builtin_namespace();
}

void System_Impl::load_library(Shared<const String> path)
{
    auto file = make<File_Script>(std::move(path), Context{});
    Program prog{*file, *this};
    prog.compile();
    auto stdlib = prog.eval();
    auto m = stdlib.to<Module>(At_Phrase(*prog.phrase_, nullptr));
    for (auto b : *m)
        std_namespace_[b.first] = make<Builtin_Value>(b.second);
}

const Namespace& System_Impl::std_namespace()
{
    return std_namespace_;
}

std::ostream& System_Impl::console()
{
    return console_;
}

} // namespace curv
