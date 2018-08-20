// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CSCRIPT_H
#define CSCRIPT_H

#include <libcurv/source.h>

struct CString_Script : public curv::Source
{
    char* buffer_;

    // buffer argument is a static string.
    CString_Script(const char* name, const char* buffer)
    :
        curv::Source(curv::make_string(name), buffer, buffer + strlen(buffer)),
        buffer_(nullptr)
    {
    }

    // buffer argument is a heap string, allocated using malloc.
    CString_Script(const char* name, char* buffer)
    :
        curv::Source(curv::make_string(name), buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}

    ~CString_Script()
    {
        if (buffer_) free(buffer_);
    }
};

#endif
