extern "C" {
#include <stdlib.h>
#include <string.h>
}
#include <aux/exception.h>
#include <aux/progdir.h>
namespace fs = boost::filesystem;

/*
 * Compute the absolute pathname of the directory
 * containing the program executable.
 * The storage for the result string is dynamically allocated.
 * argv0 is argv[0] of the argv passed to main().
 * ,,, Unix specific, right now.
 */
fs::path
aux::progdir(const char *argv0)
{
    fs::path cmd(argv0);

    if (cmd.is_absolute())
        return cmd.parent_path();

    if (cmd.has_parent_path())
        return fs::current_path() / cmd.parent_path();

    const char* PATH = getenv("PATH");
    if (PATH == NULL) {
        throw Exception(curv::stringify(
            "Can't determine directory of program ", argv0,
            ": PATH not defined"));
    }


    const char* p = PATH;
    const char* pend = PATH + strlen(PATH);
    while (p < pend) {
        const char* q = strchr(p, ':');
        if (q == nullptr)
            q = pend;
        fs::path file(p, q);
        file /= argv0;
        if (fs::exists(fs::status(file)))
            return file.parent_path();
        p = (q < pend ? q + 1 : pend);
    }

    throw Exception(curv::stringify(
        "Can't determine directory of program ", argv0,
        ": can't find ", argv0, " in $PATH"));
}
