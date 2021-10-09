// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_STRING_H
#define LIBCURV_STRING_H

#include <libcurv/alist.h>
#include <libcurv/format.h>
#include <libcurv/range.h>
#include <libcurv/value.h>
#include <sstream>
#include <cstring>

namespace curv {

/// Representation of strings and symbols in the Curv runtime.
///
/// This is a variable length object: the size and the character array
/// are in the same object. This is very efficient for small strings:
/// only a single cache hit is required to access both the size and the data.
///
/// Base is derived from Ref_Value (for the print_string override).
/// Base must define members
///     size_t size_;
///     char data_[1];
///     size()
///     empty()
template<class Base>
struct String_Mixin : public Base
{
protected:
    String_Mixin() : Base() {}
public:
    static Shared<String_Mixin>
    make(size_t len)
    {
        void* raw = malloc(sizeof(String_Mixin) + len);
        if (raw == nullptr)
            throw std::bad_alloc();
        String_Mixin* s = new(raw) String_Mixin();
        s->data_[len] = '\0';
        s->size_ = len;
        return Shared<String_Mixin>{s};
    }
    static Shared<String_Mixin>
    make(const char* str, size_t len)
    {
        void* raw = malloc(sizeof(String_Mixin) + len);
        if (raw == nullptr)
            throw std::bad_alloc();
        String_Mixin* s = new(raw) String_Mixin();
        memcpy(s->data_, str, len);
        s->data_[len] = '\0';
        s->size_ = len;
        return Shared<String_Mixin>{s};
    }
private:
    String_Mixin(const String_Mixin&) = delete;
    String_Mixin(String_Mixin&&) = delete;
    String_Mixin& operator=(const String_Mixin&) = delete;
public:
    virtual void print_string(std::ostream& out) const override
    {
        out << c_str();
    }
    // interface is based on std::string and the STL container concept
    char operator[](size_t i) const { return Base::data_[i]; }
    char at(size_t i) const { return Base::data_[i]; }
    char& at(size_t i) { return Base::data_[i]; }
    const char* data() const { return Base::data_; }
    const char* c_str() const { return Base::data_; }
    const char* begin() const { return Base::data_; }
    const char* end() const { return Base::data_ + Base::size_; }
    bool operator==(const char* s) const { return strcmp(Base::data_, s) == 0; }
    bool operator==(const String_Mixin& s) const { return strcmp(Base::data_,s.data_)==0; }
    bool operator!=(const String_Mixin& s) const { return strcmp(Base::data_,s.data_)!=0; }
    bool operator<(const String_Mixin& s) const { return strcmp(Base::data_,s.data_)<0; }
};

template <class Base>
inline std::ostream&
operator<<(std::ostream& out, const String_Mixin<Base>& str)
{
    out.write(str.data(), str.size());
    return out;
}

struct String_Base;
using String = String_Mixin<String_Base>;
struct String_Base : public Abstract_List
{
    String_Base() : Abstract_List(sty_string) {}
    virtual void print_repr(std::ostream&) const override;
    static const char name[];
    virtual Value val_at(size_t i) const override
    {
        return Value{data_[i]};
    }
    char data_[1];
};

void write_curv_string(const char*, unsigned, std::ostream&);
void write_curv_char(char c, char next, unsigned indent, std::ostream& out);

inline std::ostream&
operator<<(std::ostream& out, Shared<const String> str)
{
    out.write(str->data(), str->size());
    return out;
}

bool is_string(Value);

// Make a curv::String from an array of characters
Shared<String> make_string(const char* str, size_t len);

// Make an uninitialized curv::String
Shared<String> make_uninitialized_string(size_t len);

// Make a curv::String from a character Range
inline Shared<String> make_string(Range<const char*> r)
{
    return make_string(r.begin(), r.size());
}

// Make a curv::String from a C string
//
// If you wish to convert a `boost::filesystem::path to Shared<String>`,
// call `make_string(path.string().c_str())`.
// In particular, `make_string(path.c_str())` is not enough and fails
// compilation on Windows as `path.c_str()` will return a wchar_t* string.
//
// If you really have a pure wchar_t* string (this should only happen on
// Windows, e.g. from Windows API), do the following:
//    1. add `#ifdef _WIN32 #include <libcurv/win32.h> #endif`,
//    2. and use `make_string(wstr_to_string(my_str).c_str())`.
inline Shared<String>
make_string(const char*str)
{
    return make_string(str, strlen(str));
}

// Deleted overload, use other overloads!
//
// Rationale for explicit deletion: without deletion, calling
// make_string(wchar_t*) would use the `make_string(Value);` overload --
// which is wrong since the implicit cast to Value makes the string "true"
// (literally "true") out of wchar_t* strings, independent of their contents.
Shared<String> make_string(const wchar_t*) = delete;

/// Make a curv::String from a std::string
inline Shared<String>
make_string(const std::string& str)
{
    return make_string(str.data(), str.size());
}

// convert Value to String using Value::print_string()
Shared<const String> to_print_string(Value);

// fail if value is not a string
Shared<const String> value_to_string(Value, Fail, const Context&);

// A function that takes a string as an argument
// can declare the formal parameter as `String_Ref str`.
// All of the different string types used by libcurv are acceptable
// as arguments, and converted internally to `Shared<const String>`.
struct String_Ref : public Shared<const String>
{
    String_Ref(const char* str)
    :
        Shared<const String>(make_string(str))
    {}
    String_Ref(const std::string& str)
    :
        Shared<const String>(make_string(str.c_str(), str.size()))
    {}
    String_Ref(Range<const char*> str)
    :
        Shared<const String>(make_string(str.begin(), str.size()))
    {}
    String_Ref(Shared<const String> str)
    :
        Shared<const String>(std::move(str))
    {}
    String_Ref(Shared<String> str)
    :
        Shared<const String>(std::move(str))
    {}
    operator const char*() { return (*this)->c_str(); }
};

/// Factory class for building a curv::String using ostream operations.
struct String_Builder : public std::stringstream
{
    // An optimized version of this class would use a curv::String
    // as the internal string buffer.

    Shared<String> get_string();
    Value get_value();

    // variadic function that appends each argument to the string buffer
    template<typename First, typename... Rest>
    void write_all(First&& first, Rest&&... rest)
    {
        *this << std::forward<First>(first);
        write_all(std::forward<Rest>(rest)...);
    }

    // base case for write_all, to terminate recursive call in variadic case
    void write_all() {}
};

// Print floating point numbers accurately (to a String_Builder)
inline String_Builder&
operator<<(String_Builder& b, double n)
{
    b << dfmt(n);
    return b;
}

// Can't overload on 'double' (see above)
// without overloading on *all* the integral types.
inline String_Builder&
operator<<(String_Builder& b, long long n)
{
    (std::stringstream&)b << n;
    return b;
}
inline String_Builder&
operator<<(String_Builder& b, unsigned long long n)
{
    (std::stringstream&)b << n;
    return b;
}
inline String_Builder&
operator<<(String_Builder& b, long n)
{
    (std::stringstream&)b << n;
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
operator<<(String_Builder& b, short n)
{
    (std::stringstream&)b << n;
    return b;
}
inline String_Builder&
operator<<(String_Builder& b, unsigned short n)
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
inline String_Builder&
operator<<(String_Builder& b, signed char c)
{
    (std::stringstream&)b << c;
    return b;
}
inline String_Builder&
operator<<(String_Builder& b, unsigned char c)
{
    (std::stringstream&)b << c;
    return b;
}

// Variadic function that converts its arguments into a curv String.
template<typename... Args>
Shared<String> stringify(Args&&... args)
{
    String_Builder s;
    s.write_all(std::forward<Args>(args)...);
    return s.get_string();
}

} // namespace curv
#endif // header guard
