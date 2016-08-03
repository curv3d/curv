// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_RECORD_H
#define CURV_RECORD_H

#include <curv/atom.h>
#include <curv/value.h>

namespace curv {

/// A record value: {x=1,y=2}
struct Record : public Ref_Value
{
    Atom_Map<Value> fields_;

    Record() : Ref_Value(ty_record) {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
