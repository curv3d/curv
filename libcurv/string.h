// Copyright 2016-2020 Doug Moen
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

/// Representation of strings and symbols in the Curv runtime.
///
/// This is a variable length object: the size and the character array
/// are in the same object. This is very efficient for small strings:
/// only a single cache hit is required to access both the size and the data.
struct String_or_Symbol : public Ref_Value
{
private:
    String_or_Symbol(const String_or_Symbol&) = delete;
    String_or_Symbol(String_or_Symbol&&) = delete;
    String_or_Symbol& operator=(const String_or_Symbol&) = delete;
protected:
    String_or_Symbol(int t) : Ref_Value(t) {}
    template <class STRING>
    static Shared<STRING>
    make(int ty, size_t len)
    {
        void* raw = malloc(sizeof(STRING) + len);
        if (raw == nullptr)
            throw std::bad_alloc();
        STRING* s = new(raw) STRING(ty);
        s->data_[len] = '\0';
        s->size_ = len;
        return Shared<STRING>{s};
    }
    template <class STRING>
    static Shared<STRING>
    make(int ty, const char* str, size_t len)
    {
        void* raw = malloc(sizeof(STRING) + len);
        if (raw == nullptr)
            throw std::bad_alloc();
        STRING* s = new(raw) STRING(ty);
        memcpy(s->data_, str, len);
        s->data_[len] = '\0';
        s->size_ = len;
        return Shared<STRING>{s};
    }
private:
    size_t size_;
    char data_[1];
public:
    virtual void print_string(std::ostream&) const;
    // interface is based on std::string and the STL container concept
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    char operator[](size_t i) const { return data_[i]; }
    char at(size_t i) const { return data_[i]; }
    char& at(size_t i) { return data_[i]; }
    const char* data() const { return data_; }
    const char* c_str() const { return data_; }
    const char* begin() const { return data_; }
    const char* end() const { return data_ + size_; }
    bool operator==(const char* s) const { return strcmp(data_, s) == 0; }
    bool operator==(const String_or_Symbol& s) const { return strcmp(data_,s.data_)==0; }
    bool operator!=(const String_or_Symbol& s) const { return strcmp(data_,s.data_)!=0; }
    bool operator<(const String_or_Symbol& s) const { return strcmp(data_,s.data_)<0; }
};

struct String : public String_or_Symbol
{
    using String_or_Symbol::String_or_Symbol;
    friend Shared<String> make_string(const char*, size_t);
    friend Shared<String> make_string(size_t);
    virtual void print_repr(std::ostream&) const;
    static const char name[];
};

inline std::ostream&
operator<<(std::ostream& out, const String_or_Symbol& str)
{
    out.write(str.data(), str.size());
    return out;
}

void write_curv_string(const char*, unsigned, std::ostream&);

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
Shared<String> make_string(size_t len);

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

// return nullptr if value is not a string
Shared<const String> maybe_string(Value);

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
