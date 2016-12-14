// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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
    virtual void print(std::ostream&) const override;
    bool operator==(const Record&) const;
    virtual Value getfield(Atom) const override;
};

} // namespace curv
#endif // header guard
