// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/string.h>

namespace curv {

bool is_string(Value val)
{
    return val.maybe<String>() != nullptr;
}

Shared<const String>
value_to_string(Value val, const Context& cx)
{
    return val.to<const String>(cx);
}

Shared<const String>
maybe_string(Value val, const Context& cx)
{
    return val.maybe<const String>();
}

const char String::name[] = "string";

Shared<String>
make_string(const char* str, size_t len)
{
    return String::make<String>(Ref_Value::ty_string, str, len);
}

Shared<const String>
to_print_string(Value val)
{
    auto s = val.maybe<const String>();
    if (s) return s;
    String_Builder sb;
    val.print_string(sb);
    return sb.get_string();
}

Shared<String>
String_Builder::get_string()
{
    auto s = str(); // copies the data
    return make_string(s.data(), s.size()); // copies the data again
}

void
String_or_Symbol::print_string(std::ostream& out) const
{
    out << c_str();
}

void
String::print_repr(std::ostream& out) const
{
    write_curv_string(c_str(), 0, out);
}

void
write_curv_string(const char* s, unsigned indent, std::ostream& out)
{
    out << '"';
    for (; *s != '\0'; ++s) {
        char c = *s;
        if (c == '$')
            out << "$.";
        else if (c == '"')
            out << "$=";
        else if (c == '\n') {
            out << "\n";
            for (unsigned i = 0; i < indent; ++i)
                out << " ";
            if (s[1] != '\0')
                out << "|";
        } else
            out << c;
    }
    out << '"';
}

} // namespace curv