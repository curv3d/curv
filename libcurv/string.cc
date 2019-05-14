// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/string.h>

namespace curv {

const char String::name[] = "string";

Shared<String>
make_string(const char* str, size_t len)
{
    void* raw = malloc(sizeof(String) + len);
    if (raw == nullptr)
        throw std::bad_alloc();
    String* s = new(raw) String();
    memcpy(s->data_, str, len);
    s->data_[len] = '\0';
    s->size_ = len;
    return Shared<String>{s};
}

Shared<String>
String_Builder::get_string()
{
    auto s = str(); // copies the data
    return make_string(s.data(), s.size()); // copies the data again
}

void
String::print(std::ostream& out) const
{
    write_curv_string(data_, 0, out);
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
