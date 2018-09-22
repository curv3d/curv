// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/symbol.h>
#include <cctype>

namespace curv {

bool Symbol::is_identifier() const
{
    if (empty() || size() == 0)
        return false;
    const char* p = c_str();
    if (!(isalpha(*p) || *p == '_'))
        return false;
    for (++p; *p != '\0'; ++p)
        if (!(isalnum(*p) || *p == '_'))
            return false;
    return true;
}

std::ostream& operator<<(std::ostream& out, Symbol a)
{
    if (a.is_identifier())
        out << *a;
    else
        out << a.to_value();
    return out;
}

}
