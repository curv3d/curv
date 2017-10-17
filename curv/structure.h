// Copyright Doug Moen 2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_STRUCTURE_H
#define CURV_STRUCTURE_H

#include <curv/atom.h>
#include <curv/list.h>

namespace curv {

// A curv 'structure' is a value containing a set of fields (name/value pairs).
// Examples are records, modules and shapes.
// All structures have the same protocol for field access, which is encapsulated
// in this pure virtual class.
struct Structure : public Ref_Value
{
    Structure(int type) : Ref_Value(type) {}

    /// Get the value of a named field, throw exception if not defined.
    virtual Value getfield(Atom, const Context&) const;

    /// Test if the value contains the named field.
    virtual bool hasfield(Atom) const = 0;

    // Copy the fields into an Atom_Map.
    virtual void putfields(Atom_Map<Value>&) const = 0;

    virtual size_t size() const = 0;

    virtual Shared<List> fields() const = 0;

    static const char name[];
};

} // namespace curv
#endif // header guard
