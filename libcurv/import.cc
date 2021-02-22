// Copyright 2016-2020 Doug Moen
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

Value import(const Filesystem::path& path, const Context& cx)
{
    System& sys{cx.system()};

    // If file is a directory, use directory import.
    boost::system::error_code errcode;
    if (Filesystem::is_directory(path, errcode))
        return dir_import(path, cx);
    if (errcode)
        throw Exception(cx, stringify(path,": ",errcode.message()));

    // Construct filename extension (includes leading '.')
    std::string ext = path.extension().string();
    for (char& c : ext)
        c = std::tolower(c);

    // Import file based on extension
    auto importp = sys.importers_.find(ext);
    if (importp != sys.importers_.end())
        return (*importp->second)(path, cx);
    else {
        // If extension not recognized, it defaults to a Curv program.
        return curv_import(path, cx);
    }
}

Value curv_import(const Filesystem::path& path, const Context& cx)
{
    System& sys{cx.system()};
    auto source = make<File_Source>(make_string(path.string().c_str()), cx);
    Program prog{move(source), sys,
        Program_Opts().file_frame(cx.frame())};
    auto filekey = Filesystem::canonical(path);
    auto& active_files = sys.active_files_;
    if (active_files.find(filekey) != active_files.end())
        throw Exception{cx,
            stringify("illegal recursive reference to file ",path)};
    Active_File af(active_files, filekey);
    prog.compile();
    return prog.eval();
}

Value dir_import(const Filesystem::path& dir, const Context& cx)
{
    return {make<Dir_Record>(dir, cx)};
}

}
