// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/string.h>
#include <libcurv/list.h>
#include <libcurv/exception.h>

namespace curv {

bool is_string(Value val)
{
    if (val.maybe<String>() != nullptr) return true;
    if (auto list = val.maybe<List>()) {
        for (auto c : *list)
            if (!c.is_char()) return false;
        return true;
    }
    return false;
}

Shared<const String>
value_to_string(Value val, Fail fl, const Context& cx)
{
    if (auto str = val.maybe<const String>())
        return str;
    if (auto list = val.maybe<const List>()) {
        auto str = make_uninitialized_string(list->size());
        for (unsigned i = 0; i < list->size(); ++i) {
            Value c = list->at(i);
            if (c.is_char())
                str->at(i) = c.to_char_unsafe();
            else goto error;
        }
        return str;
    }
  error:
    FAIL(fl, nullptr, cx, stringify(val," is not a string"));
}

const char String_Base::name[] = "string";

Shared<String>
make_uninitialized_string(size_t len)
{
    return String::make(len);
}

Shared<String>
make_string(const char* str, size_t len)
{
    return String::make(str, len);
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
Value
String_Builder::get_value()
{
    auto s = str(); // copies the data
    if (s.empty()) return {make_tail_array<List>(0)};
    return {make_string(s.data(), s.size())}; // copies the data again
}

void
String_Base::print_repr(std::ostream& out, Prec) const
{
    write_curv_string(data_, 0, out);
}

void
write_curv_string(const char* s, unsigned indent, std::ostream& out)
{
    // Normally, a String should not be empty, but be robust and handle anyway.
    if (*s == '\0') {
        out << "[]";
        return;
    }
    out << '"';
    for (; *s != '\0'; ++s) {
        char c = *s;
        if (c == '$') {
            out << c;
            if (is_dollar_next_char(s[1]))
                out << '_';
        }
        else if (c == '"')
            out << "\"_";
        else if (c == '\n') {
            out << "\n";
            for (unsigned i = 0; i < indent; ++i)
                out << " ";
            if (s[1] != '\0')
                out << "|";
        } else if (c == '\t')
            out << c;
        else if (c < ' ' || c > '~')
            out << "$[" << unsigned(c) << "]";
        else
            out << c;
    }
    out << '"';
}

void
write_curv_char(char c, char next, unsigned indent, std::ostream& out)
{
    if (c == '$') {
        out << c;
        if (is_dollar_next_char(next))
            out << '_';
    }
    else if (c == '"')
        out << "\"_";
    else if (c == '\n') {
        out << "\n";
        for (unsigned i = 0; i < indent; ++i)
            out << " ";
        if (next != '\0')
            out << "|";
    } else if (c == '\t')
        out << c;
    else if (c < ' ' || c > '~')
        out << "$[" << unsigned(c) << "]";
    else
        out << c;
}

} // namespace curv
