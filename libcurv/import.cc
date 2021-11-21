// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/import.h>

#include <libcurv/context.h>
#include <libcurv/dir_record.h>
#include <libcurv/exception.h>
#include <libcurv/program.h>
#include <libcurv/system.h>
#include <cstdlib>

namespace curv {

void import(const Filesystem::path& path, Program& prog, const Context& cx)
{
    System& sys{cx.system()};

    // If file is a directory, use directory import.
    std::error_code errcode;
    if (Filesystem::is_directory(path, errcode))
        return dir_import(path, prog, cx);
    if (errcode)
        throw Exception(cx, stringify(path,": ",errcode.message()));

    // Construct filename extension (includes leading '.')
    std::string ext = path.extension().string();
    for (char& c : ext)
        c = std::tolower(c);

    // Import file based on extension
    auto importp = sys.importers_.find(ext);
    if (importp != sys.importers_.end())
        return (*importp->second)(path, prog, cx);
    else {
        // If extension not recognized, it defaults to a Curv program.
        return curv_import(path, prog, cx);
    }
}

Value
import_value(Importer imp, const Filesystem::path& path, const Context& cx)
{
    std::error_code errcode;
    auto filekey = Filesystem::canonical(path, errcode);
    if (errcode)
        throw Exception(cx, stringify(path,": ",errcode.message()));
    auto& active_files = cx.system().active_files_;
    if (active_files.find(filekey) != active_files.end())
        throw Exception{cx,
            stringify("illegal recursive reference to file ",path)};
    Active_File af(active_files, filekey);

    Program prog(cx.system(), cx.frame());
    imp(path, prog, cx);
    return prog.eval();
}

void curv_import(const Filesystem::path& path, Program& prog, const Context& cx)
{
    prog.compile(make<File_Source>(path.string(), cx));
}

void dir_import(const Filesystem::path& dir, Program& prog, const Context& cx)
{
    Value val = {make<Dir_Record>(dir, cx)};
    prog.compile(dir, Source::Type::directory, val);
}

}
