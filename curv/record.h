// Copyright Doug Moen 2016-2017.
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
    Record(Atom_Map<Value> fields)
    :
        Ref_Value(ty_record),
        fields_(std::move(fields))
    {
    }

    Shared<const Record> clone() const
    {
        return make<const Record>(fields_);
    }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;
    bool operator==(const Record&) const;
    virtual Value getfield(Atom, const Context&) const override;
    virtual bool hasfield(Atom) const override;

    static const char name[];
};

} // namespace curv
#endif // header guard
