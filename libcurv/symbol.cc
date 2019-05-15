// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/symbol.h>
#include <cctype>

namespace curv {

const char Symbol::name[] = "symbol";

Shared<const Symbol>
make_symbol(const char* str, size_t len)
{
    return Symbol::make<Symbol>(Ref_Value::ty_symbol, str, len);
}

bool is_C_identifier(const char* p)
{
    if (!(isalpha(*p) || *p == '_'))
        return false;
    for (++p; *p != '\0'; ++p)
        if (!(isalnum(*p) || *p == '_'))
            return false;
    return true;
}

bool Symbol_Ref::is_identifier() const
{
    if (empty())
        return false;
    return is_C_identifier(c_str());
}

Shared<const Symbol> token_to_symbol(Range<const char*> str)
{
    if (str[0] == '\'') {
        // parse the quoted identifier
        assert(str.size() >= 2);
        assert(str[str.size()-1] == '\'');
        String_Builder sb;
        for (const char* p = &str[1]; *p != '\''; ++p) {
            if (*p == '$') {
                ++p;
                if (*p == '-')
                    sb << '\'';
                else if (*p == '.')
                    sb << '$';
            } else
                sb << *p;
        }
        return make_symbol(sb.str());
    } else {
        return make_symbol(str);
    }
}

// Construct a symbol from an identifier token, which may be quoted.
Symbol_Ref::Symbol_Ref(Range<const char*> str, bool)
:
    Base(token_to_symbol(str))
{
}

std::ostream& operator<<(std::ostream& out, Symbol_Ref a)
{
    if (a.is_identifier())
        out << *a;
    else {
        out << '\'';
        for (const char* s = a.c_str(); *s != '\0'; ++s) {
            char c = *s;
            if (c == '$')
                out << "$.";
            else if (c == '\'')
                out << "$-";
            else
                out << c;
        }
        out << '\'';
    }
    return out;
}

void
Symbol::print(std::ostream& out) const
{
    out << "#" << Symbol_Ref(share(*this));
}

}
