// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_STRING_H
#define LIBCURV_STRING_H

#include <libcurv/value.h>
#include <sstream>
#include <cstring>
#include <libcurv/dtostr.h>
#include <libcurv/range.h>

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
    friend Shared<String> make(const char*, size_t);
    String(const String&) = delete;
    String(String&&) = delete;
    String& operator=(const String&) = delete;
private:
    size_t size_;
    char data_[1];
public:
    /// Make a curv::String from an array of characters
    static Shared<String> make(const char*, size_t);
    inline static Shared<String> make(Range<const char*> r)
    {
        return make(r.begin(), r.size());
    }

    // interface is based on std::string and the STL container concept
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    char operator[](size_t i) const { return data_[i]; }
    char at(size_t i) const { return data_[i]; }
    const char* data() const { return data_; }
    const char* c_str() const { return data_; }
    const char* begin() const { return data_; }
    const char* end() const { return data_ + size_; }
    bool operator==(const char* s) const { return strcmp(data_, s) == 0; }
    bool operator==(const String& s) const { return strcmp(data_,s.data_)==0; }
    bool operator!=(const String& s) const { return strcmp(data_,s.data_)!=0; }
    bool operator<(const String& s) const { return strcmp(data_,s.data_)<0; }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
    static const char name[];
};

inline std::ostream&
operator<<(std::ostream& out, const String& str)
{
    out.write(str.data(), str.size());
    return out;
}

inline std::ostream&
operator<<(std::ostream& out, Shared<const String> str)
{
    out.write(str->data(), str->size());
    return out;
}

/// Make a curv::String from an array of characters
inline Shared<String> make_string(const char* str, size_t len)
{
    return String::make(str, len);
}

/// Make a curv::String from a C string
inline Shared<String>
make_string(const char*str)
{
    return String::make(str, strlen(str));
}

/// Factory class for building a curv::String using ostream operations.
struct String_Builder : public std::stringstream
{
    // An optimized version of this class would use a curv::String
    // as the internal string buffer.

    Shared<String> get_string();

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
    b << dfmt(n);
    return b;
}

inline String_Builder&
operator<<(String_Builder& b, unsigned long n)
{
    (std::stringstream&)b << n;
    return b;
}

inline String_Builder&
operator<<(String_Builder& b, int n)
{
    (std::stringstream&)b << n;
    return b;
}

inline String_Builder&
operator<<(String_Builder& b, unsigned n)
{
    (std::stringstream&)b << n;
    return b;
}

inline String_Builder&
operator<<(String_Builder& b, char c)
{
    (std::stringstream&)b << c;
    return b;
}

/// Variadic function that converts its arguments into a curv String.
template<typename... Args>
Shared<String> stringify(Args... args)
{
    String_Builder s;
    s.write_all(args...);
    return s.get_string();
}

} // namespace curv
#endif // header guard
