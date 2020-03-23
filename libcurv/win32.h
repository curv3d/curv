#ifndef _WIN32
    #error "You must not include win32.h on non-Windows platforms. Use #ifdefs with _WIN32"
#endif

#ifndef LIBCURV_WIN32_H
#define LIBCURV_WIN32_H

#include <string>

extern "C" {
    #include <Windows.h>
}

namespace curv {
    // Convert a UTF-16-encoded wide string to a UTF8 string using Windows API
    //
    // Even though wchar_t* strings also exists on OSes other than Windows,
    // it is not guaranteed that the underlying wchar_t wide characters
    // are 16 bit large nor might it be the (default) case that wchar_t*
    // strings are UTF-16-encoded.
    // Hence, we only expose this function on Windows.
    std::string wstr_to_string(const wchar_t* wstr);

    // Convert a UTF-8 encoded string to a UTF-16-encoded wide string using Windows API
    //
    // Generally avoid usage of std::wstring and wchar_t* strings in Curv.
    // Therefore, this function makes most sense to use if you need to interfere with an
    // external API such as Windows API.
    //
    // The same remarks as for wstr_to_string as for why this function is only exposed on
    // Windows hold true here as well.
    std::wstring str_to_wstring(const char* str);

    // Format an error code from the Windows API (e.g. as obtained via GetLastError) as a std::string
    std::string win_strerror(DWORD error_code);

} // namespace curv
#endif // header guard