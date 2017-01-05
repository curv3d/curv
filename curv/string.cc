// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/string.h>

using namespace aux;
namespace curv {

Shared<String>
String::make(const char* str, size_t len)
{
    void* raw = malloc(sizeof(String) + len);
    if (raw == nullptr)
        throw std::bad_alloc();
    String* s = new(raw) String();
    memcpy(s->data_, str, len);
    s->data_[len] = '\0';
    s->size_ = len;
    return share(*s);
}

Shared<String>
String_Builder::get_string()
{
    return String::make(str().data(), str().size());
}

void
String::print(std::ostream& out) const
{
    out << '"';
    for (size_t i = 0; i < size_; ++i) {
        char c = data_[i];
        if (c == '$' || c == '"')
            out << '$';
        out << c;
    }
    out << '"';
}

} // namespace curv
