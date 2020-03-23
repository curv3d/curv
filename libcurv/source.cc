// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/source.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/filesystem.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

namespace curv
{

Shared<const String> readfile(const char* path, const Context& ctx)
{
    // TODO: Cache multiple references to the same file. Where does the cache
    // go? System or Session object.

    // TODO: Pluggable file system abstraction, for unit testing and
    // abstracting the behaviour of `file` (would also support caching).
    // So, use System::open(path)? Maybe this returns a Source?

    // TODO: More precise error message when open fails. Maybe get that
    // from an exception? No, the following code is useless, no useful
    // information is stored in what() or code().message().
    /*
    ifstream file;
    file.exceptions ( ifstream::failbit | ifstream::badbit );
    try {
      file.open ("test.txt");
      while (!file.eof()) file.get();
    }
    catch (const ifstream::failure& e) {
      cout << "Exception opening/reading file\n";
      cout << e.what() << "\n";
      cout << e.code().message() << "\n";
    }
    */
    // I'll need to use strerror(errno).

    // Don't use mmap. If source file is on a remote networked file system,
    // and network disconnects, you get a SIGBUS when reading file memory.
    // Handling SIGBUS correctly is extremely complex and platform dependent.
    // https://www.sublimetext.com/blog/articles/use-mmap-with-care

    std::ifstream t;
    t.open(path);
    if (t.fail())
        throw Exception(ctx,
            stringify("\"", path, "\": ", strerror(errno)));
    std::stringstream buffer;
    buffer << t.rdbuf();
    return make_string(buffer.str());
}

File_Source::File_Source(String_Ref filename, const Context& ctx)
:
    String_Source(filename, readfile(filename->c_str(), ctx))
{
    if (Filesystem::path(std::string(filename)).extension() == ".gpu")
        type_ = Type::gpu;
}

} // namespace curv
