// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SYMBOL_H
#define LIBCURV_SYMBOL_H

#include <libcurv/string.h>
#include <map>
#include <vector>

namespace curv {

bool is_symbol(Value a);

struct Symbol_Ref;

struct Symbol : public String_or_Symbol
{
    using String_or_Symbol::String_or_Symbol;
    // you must call make_symbol() to construct a Symbol.
    friend Symbol_Ref make_symbol(const char*, size_t);
    virtual void print_repr(std::ostream&) const;
    static const char name[];
};

/// A Symbol_Ref is a short immutable string with an efficient representation.
///
/// A Symbol_Ref represents an identifier during semantic analysis and run time.
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
struct Symbol_Ref : private Shared<const Symbol>
{
private:
    using Base = Shared<const Symbol>;
    inline Symbol_Ref(Base sym) : Base(sym) {}
    friend void Symbol::print_repr(std::ostream& out) const;
public:
    using Shared<const Symbol>::Shared;

    inline Symbol_Ref()
    :
        Base()
    {
    }

    inline Symbol_Ref& operator=(Symbol_Ref a2)
    {
        Base::operator=(a2);
        return *this;
    }

    bool empty() const noexcept
    {
        return this->get() == nullptr;
    }

    int cmp(Symbol_Ref a) const noexcept
    {
        return strcmp((*this)->c_str(), a->c_str());
    }
    friend bool operator==(Symbol_Ref a1, Symbol_Ref a2) noexcept
    {
        return *a1 == *a2;
    }
    friend bool operator==(Symbol_Ref a1, const char* a2) noexcept
    {
        return strcmp(a1->c_str(), a2) == 0;
    }
    friend bool operator!=(Symbol_Ref a1, Symbol_Ref a2) noexcept
    {
        return !(*a1 == *a2);
    }
    friend bool operator<(Symbol_Ref a1, Symbol_Ref a2) noexcept
    {
        return *a1 < *a2;
    }

  #if 0
    inline const char* data() const { return (*this)->data(); }
    inline char operator[](size_t i) const { return (**this)[i]; }
  #endif
    inline size_t size() const { return (*this)->size(); }
    inline const char* c_str() const { return (*this)->c_str(); }

    friend void swap(Symbol_Ref& a1, Symbol_Ref& a2) noexcept
    {
        a1.swap(a2);
    }
    friend std::ostream& operator<<(std::ostream& out, Symbol_Ref a);
    Value to_value() const;
    bool is_identifier() const;
};

Symbol_Ref make_symbol(const char*, size_t);

inline Symbol_Ref make_symbol(const char* s)
{
    return make_symbol(s, strlen(s));
}

inline Symbol_Ref make_symbol(std::string s)
{
    return make_symbol(s.data(), s.size());
}

Symbol_Ref token_to_symbol(Range<const char*> str);

Symbol_Ref value_to_symbol(Value);
Symbol_Ref value_to_symbol(Value, const Context&);

bool is_C_identifier(const char*);

int value_to_enum(Value, const std::vector<const char*>&, const Context&);

/// A Symbol_Map<T> is a map from Symbol_Ref to T.
///
/// When you iterate over a Symbol_Map, the entries are produced in order,
/// according to the global ordering on Symbols. This is used to efficiently
/// merge two symbol maps.
///
/// Alternate design: https://en.wikipedia.org/wiki/Hash_array_mapped_trie,
/// with the keys ordered by hash value instead of alphabetically.
template<typename T>
struct Symbol_Map : public std::map<Symbol_Ref, T>
{
    using std::map<Symbol_Ref,T>::map;
};

} // namespace curv
#endif // header guard
