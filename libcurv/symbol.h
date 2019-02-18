// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SYMBOL_H
#define LIBCURV_SYMBOL_H

#include <map>
#include <libcurv/string.h>

namespace curv {

/// An Symbol is a short immutable string with an efficient representation.
///
/// An Symbol represents an identifier during semantic analysis and run time.
/// For example, a symbol in a symbol map, or a field name in a record value.
///
/// There is a guaranteed global ordering on symbols, which is relied on
/// for efficiently merging two symbol maps.
///
/// Possible changes in future revisions:
/// * Add a hash code, so that we can use hash trees for Symbol_Map.
/// * Use a global symbol table stored in the curv::System object to ensure that
///   symbols are unique, so we can use pointer equality as symbol equality.
///   This will also eliminate refcount manipulation, at a cost: the symbol
///   table slowly grows, never shrinks.
struct Symbol : private Shared<const String>
{
private:
    using Base = Shared<const String>;
public:
    using Shared<const String>::Shared;
    inline Symbol(const char* str)
    :
        Base(make_string(str))
    {}
    inline Symbol(Shared<const String> str)
    :
        Base(std::move(str))
    {}
    inline Symbol(String_Ref str)
    :
        Base(str)
    {}
    inline Symbol& operator=(Symbol a2)
    {
        Base::operator=(a2);
        return *this;
    }

    inline Symbol()
    :
        Base()
    {
    }
    bool empty() const noexcept
    {
        return this->get() == nullptr;
    }

    int cmp(Symbol a) const noexcept
    {
        return strcmp((*this)->c_str(), a->c_str());
    }
    friend bool operator==(Symbol a1, Symbol a2) noexcept
    {
        return *a1 == *a2;
    }
    friend bool operator==(Symbol a1, const char* a2) noexcept
    {
        return strcmp(a1->c_str(), a2) == 0;
    }
    friend bool operator!=(Symbol a1, Symbol a2) noexcept
    {
        return !(*a1 == *a2);
    }
    friend bool operator<(Symbol a1, Symbol a2) noexcept
    {
        return *a1 < *a2;
    }

    inline size_t size() const { return (*this)->size(); }
    inline const char* data() const { return (*this)->data(); }
    inline const char* c_str() const { return (*this)->c_str(); }
    inline char operator[](size_t i) const { return (**this)[i]; }

    friend void swap(Symbol& a1, Symbol& a2) noexcept
    {
        a1.swap(a2);
    }
    friend std::ostream& operator<<(std::ostream& out, Symbol a);

    Value to_value() const
    {
        // Currently, we copy the string data, because a Symbol is immutable,
        // but a Value can only be constructed from a mutable String reference.
        return {String::make(data(), size())};
    }
    bool is_identifier() const;
};

bool is_C_identifier(const char*);

/// A Symbol_Map<T> is a map from Symbol to T.
///
/// When you iterate over a Symbol_Map, the entries are produced in order,
/// according to the global ordering on Symbols. This is used to efficiently
/// merge two symbol maps.
///
/// Alternate design: https://en.wikipedia.org/wiki/Hash_array_mapped_trie,
/// with the keys ordered by hash value instead of alphabetically.
template<typename T>
struct Symbol_Map : public std::map<Symbol, T>
{
    using std::map<Symbol,T>::map;
};

} // namespace curv
#endif // header guard
