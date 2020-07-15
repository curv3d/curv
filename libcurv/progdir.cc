extern "C" {
#include <stdlib.h>
#include <string.h>
}
#include <libcurv/exception.h>
#include <libcurv/progdir.h>
namespace fs = boost::filesystem;

namespace curv {

/*
 * Compute the absolute pathname of the directory
 * containing the program executable.
 * The storage for the result string is dynamically allocated.
 * argv0 is argv[0] of the argv passed to main().
 */
fs::path
progdir(const char *argv0)
{
    fs::path cmd(argv0);
#ifdef _WIN32
    cmd += ".exe";
#endif

    if (cmd.has_parent_path()) {
        return fs::canonical(cmd).parent_path();
    }

    const char* PATH = getenv("PATH");
    if (PATH == NULL) {
        throw curv::Exception_Base(curv::stringify(
            "Can't determine directory of program ", argv0,
            ": PATH not defined"));
    }


    const char* p = PATH;
    const char* pend = PATH + strlen(PATH);
#ifdef _WIN32
    const char delim = ';';
#else
    const char delim = ':';
#endif
    while (p < pend) {
        const char* q = strchr(p, delim);
        if (q == nullptr)
            q = pend;
        fs::path file(p, q);
        file /= cmd;
        if (fs::exists(fs::status(file))) {
            return fs::canonical(file).parent_path();
        }
        p = (q < pend ? q + 1 : pend);
    }

    throw curv::Exception_Base(curv::stringify(
        "Can't determine directory of program ", argv0,
        ": can't find ", argv0, " in $PATH"));
}

}
