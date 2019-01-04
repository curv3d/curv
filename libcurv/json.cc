// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/json.h>

namespace curv {

void write_json_string(const char* str, std::ostream& out)
{
    out << '"';
    for (const char* p = str; *p != '\0'; ++p) {
        // In the JSON-API protocol, top level objects are separated by
        // newlines, and for ease of parsing by the client, top level objects
        // cannot contain raw newlines. So newlines are encoded as \n in
        // JSON strings.
        if (*p == '\n')
            out << "\\n";
        else {
            if (*p == '\\' || *p == '"')
                out << '\\';
            out << *p;
        }
    }
    out << '"';
}

} // namespace curv
