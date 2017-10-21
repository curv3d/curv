// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ATOM_H
#define CURV_ATOM_H

#include <map>
#include <curv/string.h>

namespace curv {

/// An Atom is a short immutable string with an efficient representation.
///
/// An Atom represents an identifier during semantic analysis and run time.
/// For example, a symbol in a symbol table, or a field name in a record value.
/// The name comes from Lisp.
///
/// Possible changes in future revisions:
/// * Add a hash code, so that we can use hash trees for Atom_Map.
/// * Use a global atom table stored in the curv::Session object to ensure
///   that atoms are unique, so we can use pointer equality as atom equality.
///   This will also eliminate refcount manipulation, at a cost: the atom
///   table slowly grows, never shrinks.
struct Atom : private Shared<const String>
{
private:
    using Base = Shared<const String>;
public:
    using Shared<const String>::Shared;
    inline Atom(Shared<const String> str)
    :
        Base(str)
    {}
    inline Atom(const char* str)
    :
        Base(String::make(str, strlen(str)))
    {}
    inline Atom(const char* str, size_t len)
    :
        Base(String::make(str, len))
    {}
    inline Atom(aux::Range<const char*> str)
    :
        Base(String::make(str.begin(), str.size()))
    {}
    inline Atom& operator=(Atom a2)
    {
        Base::operator=(a2);
        return *this;
    }

    friend bool operator==(Atom a1, Atom a2) noexcept
    {
        return *a1 == *a2;
    }
    friend bool operator!=(Atom a1, Atom a2) noexcept
    {
        return !(*a1 == *a2);
    }
    friend bool operator<(Atom a1, Atom a2) noexcept
    {
        return *a1 < *a2;
    }

    inline size_t size() const { return (*this)->size(); }
    inline const char* data() const { return (*this)->data(); }
    inline char operator[](size_t i) const { return (**this)[i]; }

    friend void swap(Atom& a1, Atom& a2) noexcept
    {
        a1.swap(a2);
    }
    friend std::ostream& operator<<(std::ostream& out, Atom a)
    {
        //out << "`" << *a << "`";
        out << *a;
        return out;
    }

    Value to_value() const
    {
        // Currently, we copy the string data, because an Atom is immutable,
        // but a Value can only be constructed from a mutable String reference.
        return {String::make(data(), size())};
    }
};

/// An Atom_Map is a map from Atom to T.
/// It's supposed to be a persistent functional data structure with
/// an efficient merge operation, and that's not implemented yet.
template<typename T>
struct Atom_Map : public std::map<Atom, T>
{
    using std::map<Atom,T>::map;
};

} // namespace curv
#endif // header guard
