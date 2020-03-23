#ifdef _WIN32

#include <libcurv/win32.h>
#include <libcurv/exception.h>

namespace curv {

std::string
wstr_to_string(const wchar_t *wstr_)
{
    // TODO: if were to avoid this std::wstring creation, how would we patch wstr.size() then?
    //       Certainly, strlen would be wrong, read the WideCharToMultiByte API documentation for that
    std::wstring wstr = wstr_;

    // Copied and modified from original source: <https://stackoverflow.com/a/3999597>
    //   with author: tfinniga <https://stackoverflow.com/users/9042/tfinniga>
    //   with license: CC BY-SA 3.0 <https://creativecommons.org/licenses/by-sa/3.0/>

    if (wstr.empty())
        return std::string();

    // query wstring length first (!= byte count in general)
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int) wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed == 0)
    {
        // given that we check for emptyness above, a return value of 0 must indicate an error here
        DWORD error = GetLastError();
        throw curv::Exception_Base(stringify("Error while preparing for converting wchar_t* to char*: ", win_strerror(error)));
    }

    std::string strTo(size_needed, 0);
    // now convert
    if (WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int) wstr.size(), &strTo[0], size_needed, NULL, NULL) == 0) {
        DWORD error = GetLastError();
        throw curv::Exception_Base(stringify("Error while converting wchar_t* to char*: ", win_strerror(error)));
    }
    return strTo;
}

std::wstring
str_to_wstring(const char* str)
{
    if (str[0] == '\0')
    {
        return std::wstring();
    }

    // query required wstring length first (!= byte count in general, i.e. != strlen(str))
    int size_needed = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, -1, NULL, 0);
    if (size_needed == 0)
    {
        // given that we check for emptyness above, a return value of 0 must indicate an error here
        DWORD error = GetLastError();
        throw curv::Exception_Base(stringify("Error while preparing for converting char_t* to wchar_t*: ", win_strerror(error)));
    }

    std::wstring strTo(size_needed, 0);
    // now convert
    if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str, -1, &strTo[0], size_needed) == 0) {
        DWORD error = GetLastError();
        throw curv::Exception_Base(stringify("Error while converting char_t* to wchar_t*: ", win_strerror(error)));
    }
    return strTo;
}

std::string win_strerror(DWORD errorCode) {
    // Copied and modified from original source: <https://stackoverflow.com/a/17386923>
    //   with author: LeviX <https://stackoverflow.com/users/747145/levix>
    //   with license: CC BY-SA 3.0

    // FormatMessageW also offers to allocate a buffer of suitable size itself,
    // this probably incurs some performance penalties, so we just use a statically-sized
    // buffer.
    wchar_t buf[256];
    DWORD wideCharsWritten = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

    if (wideCharsWritten == 0)
    {
        DWORD error = GetLastError();
        // To err on the safe side, do *not* format this error code by recursively calling win_strerror
        throw curv::Exception_Base(stringify("Error while formatting WinAPI error code as message using FormatMessageW, WinAPI error code: ", error));
    }

    return wstr_to_string(buf);
}

} // namespace curv

#endif // _WIN32 guard