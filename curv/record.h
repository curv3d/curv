// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_RECORD_H
#define CURV_RECORD_H

#include <curv/atom.h>
#include <curv/structure.h>

namespace curv {

/// A record value: {x=1,y=2}
struct Record : public Structure
{
    Atom_Map<Value> fields_;

    Record() : Structure(ty_record) {}
    Record(Atom_Map<Value> fields)
    :
        Structure(ty_record),
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
    virtual void putfields(Atom_Map<Value>&) const override;
    virtual Shared<List> fields() const override;
    virtual size_t size() const override { return fields_.size(); }
    virtual void each_field(std::function<void(Atom,Value)>) const override;

    static const char name[];
};

} // namespace curv
#endif // header guard
