// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/system.h>

#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/program.h>
#include <libcurv/source.h>

namespace curv {

void System::print_exception(
    const char* prefix, const std::exception& exc,
    std::ostream& out, bool use_colour)
{
    const Exception *e = dynamic_cast<const Exception*>(&exc);
    if (e) {
        if (use_colour) out << AC_MESSAGE;
        out << prefix;
        if (use_colour) out << AC_RESET;
        e->write(out, use_colour);
    } else {
        if (use_colour) out << AC_MESSAGE;
        out << prefix << exc.what();
        if (use_colour) out << AC_RESET;
    }
    out << std::endl;
}

System_Impl::System_Impl(std::ostream& console)
:
    console_(console)
{
    std_namespace_ = builtin_namespace();
}

void System_Impl::load_library(String_Ref path)
{
    auto file = make<File_Source>(std::move(path), Context{});
    Program prog{std::move(file), *this};
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
