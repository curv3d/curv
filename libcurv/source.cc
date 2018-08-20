// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/source.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>

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

    // TODO: change Source_File to use mmap?

    std::ifstream t;
    t.open(path);
    if (t.fail())
        throw Exception(ctx,
            stringify("can't open file \"", path, "\": ", strerror(errno)));
    std::stringstream buffer;
    buffer << t.rdbuf();
    return make_string(buffer.str());
}

Source_File::Source_File(String_Ref filename, const Context& ctx)
:
    Source_String(filename, readfile(filename->c_str(), ctx))
{
}

} // namespace curv
