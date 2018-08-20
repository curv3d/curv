// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/system.h>

#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/file.h>
#include <libcurv/program.h>

namespace curv {

void System::message(const char* prefix, const std::exception& exc)
{
    const Exception *e = dynamic_cast<const Exception*>(&exc);
    if (e) {
        if (use_colour_) console() << AC_MESSAGE;
        console() << prefix;
        if (use_colour_) console() << AC_RESET;
        e->write(console(), use_colour_);
    } else {
        if (use_colour_) console() << AC_MESSAGE;
        console() << prefix << exc.what();
        if (use_colour_) console() << AC_RESET;
    }
    console() << std::endl;
}

System_Impl::System_Impl(std::ostream& console)
:
    console_(console)
{
    std_namespace_ = builtin_namespace();
}

void System_Impl::load_library(Shared<const String> path)
{
    auto file = make<Source_File>(std::move(path), Context{});
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
