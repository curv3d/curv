// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/system.h>

#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/import.h>
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

static void export_json_string(const char* str, std::ostream& out)
{
    out << '"';
    for (const char* p = str; *p != '\0'; ++p) {
        if (*p == '\\' || *p == '"')
            out << '\\';
        out << *p;
    }
    out << '"';
}

void System::print_json_exception(
    const char* type, const std::exception& exc, std::ostream& out)
{
    out << "{\"" << type << "\":{";
    out << "\"message\":";
    export_json_string(exc.what(), out);
    const Exception *e = dynamic_cast<const Exception*>(&exc);
    if (e) {
        // print location array
    }
    out << "}}" << std::endl;
}

void System::error(const std::exception& exc)
{
    if (use_json_api_)
        print_json_exception("error", exc, console());
    else
        print_exception("ERROR: ", exc, console(), use_colour_);
}

void System::warning(const std::exception& exc)
{
    if (use_json_api_)
        print_json_exception("warning", exc, console());
    else
        print_exception("WARNING: ", exc, console(), use_colour_);
}

void System::print(const char* str)
{
    if (use_json_api_) {
        console() << "{\"print\":";
        export_json_string(str, console());
        console() << "}";
    } else {
        console() << str;
    }
    console() << std::endl;
}

System_Impl::System_Impl(std::ostream& console)
:
    console_(console)
{
    std_namespace_ = builtin_namespace();
    importers_[".curv"] = curv_import;
}

void System_Impl::load_library(String_Ref path)
{
    auto file = make<File_Source>(std::move(path), At_System{*this});
    Program prog{std::move(file), *this};
    prog.compile();
    auto stdlib = prog.eval();
    auto m = stdlib.to<Module>(At_Phrase(*prog.phrase_, *this, nullptr));
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
