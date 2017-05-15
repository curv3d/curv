// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <fstream>
#include <curv/context.h>
#include <curv/exception.h>
#include <curv/file.h>

namespace curv
{

Shared<const String> readfile(const char* path, const Context& ctx)
{
    // TODO: Cache multiple references to the same file. Where does the cache
    // go? System or Session object.

    // TODO: Pluggable file system abstraction, for unit testing and
    // abstracting the behaviour of `file` (would also support caching).
    // So, use System::open(path)? Maybe this returns a Script?

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

    // TODO: change File_Script to use mmap?

    std::ifstream t;
    t.open(path);
    if (t.fail())
        throw Exception(ctx, stringify("can't open file ", path));
    String_Builder buffer;
    buffer << t.rdbuf();
    return buffer.get_string();
}

File_Script::File_Script(Shared<const String> filename, const Context& ctx)
:
    String_Script(filename, readfile(filename->c_str(), ctx))
{
}

} // namespace curv
