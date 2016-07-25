// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_STRING_H
#define CURV_STRING_H

#include <curv/value.h>
#include <sstream>
#include <cstring>
#include <aux/dtostr.h>
#include <aux/range.h>

namespace curv {

/// Representation of strings in the Curv runtime.
///
/// This is a variable length object: the size and the character array
/// are in the same object. This is very efficient for small strings:
/// only a single cache hit is required to access both the size and the data.
struct String : public Ref_Value
{
private:
    // you must call make() to construct a String.
    String() : Ref_Value(ty_string) {}
    friend aux::Shared_Ptr<String> make(const char*, size_t);
    String(const String&) = delete;
    String(String&&) = delete;
    String& operator=(const String&) = delete;
private:
    size_t size_;
    char data_[1];
public:
    /// Make a curv::String from an array of characters
    static aux::Shared_Ptr<String> make(const char*, size_t);
    inline static aux::Shared_Ptr<String> make(aux::Range<const char*> r)
    {
        return make(r.begin(), r.size());
    }

    // interface is based on std::string and the STL container concept
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    const char* data() const { return data_; }
    const char* c_str() const { return data_; }
    const char* begin() const { return data_; }
    const char* end() const { return data_ + size_; }
    bool operator==(const char* s) const { return strcmp(data_, s) == 0; }
    bool operator==(const String& s) const { return strcmp(data_,s.data_)==0; }
    bool operator<(const String& s) const { return strcmp(data_,s.data_)<0; }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

inline std::ostream&
operator<<(std::ostream& out, const String& str)
{
    out.write(str.data(), str.size());
    return out;
}

/// Make a curv::String from an array of characters
inline aux::Shared_Ptr<String> mk_string(const char* str, size_t len)
{
    return String::make(str, len);
}

/// Make a curv::String from a C string
inline aux::Shared_Ptr<String>
mk_string(const char*str)
{
    return String::make(str, strlen(str));
}

/// Factory class for building a curv::String using ostream operations.
struct String_Builder : public std::stringstream
{
    // An optimized version of this class would use a curv::String
    // as the internal string buffer.

    aux::Shared_Ptr<String> get_string();

    // variadic function that appends each argument to the string buffer
    template<typename First, typename... Rest>
    void write_all(First first, Rest... rest)
    {
        *this << first;
        write_all(rest...);
    }

    // base case for write_all, to terminate recursive call in variadic case
    void write_all() {}
};

/// Print floating point numbers accurately (to a String_Builder)
inline String_Builder&
operator<<(String_Builder& b, double n)
{
    b << aux::dfmt(n);
    return b;
}

inline String_Builder&
operator<<(String_Builder& b, unsigned long n)
{
    (std::stringstream&)b << n;
    return b;
}

/// Variadic function that converts its arguments into a curv String.
template<typename... Args>
aux::Shared_Ptr<String> stringify(Args... args)
{
    String_Builder s;
    s.write_all(args...);
    return s.get_string();
}

} // namespace curv
#endif // header guard
