// Copyright 2017-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_STRUCTURE_H
#define LIBCURV_STRUCTURE_H

#include <libcurv/symbol.h>
#include <libcurv/list.h>

namespace curv {

// A curv 'structure' is a value containing a set of fields (name/value pairs).
// Subtypes are Record and Module.
// All structures have the same protocol for field access, which is encapsulated
// in this pure virtual class.
struct Structure : public Ref_Value
{
    Structure(int subtype) : Ref_Value(ty_record, subtype) {}

    /// Get the value of a named field, throw exception if not defined.
    virtual Value getfield(Symbol, const Context&) const;

    /// Test if the value contains the named field.
    virtual bool hasfield(Symbol) const = 0;

    // Copy the fields into an Symbol_Map.
    virtual void putfields(Symbol_Map<Value>&) const = 0;

    virtual Shared<List> fields() const = 0;

    virtual size_t size() const = 0;

    // visit each field
    virtual void each_field(std::function<void(Symbol,Value)>) const = 0;

    bool equal(const Structure&, const Context&) const;

    static const char name[];
};

} // namespace curv
#endif // header guard
