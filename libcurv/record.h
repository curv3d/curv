// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_RECORD_H
#define LIBCURV_RECORD_H

#include <libcurv/symbol.h>
#include <libcurv/structure.h>

namespace curv {

/// A record value: {x=1,y=2}
struct Record : public Structure
{
    Symbol_Map<Value> fields_;

    Record() : Structure(ty_record) {}
    Record(Symbol_Map<Value> fields)
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
    virtual Value getfield(Symbol, const Context&) const override;
    virtual bool hasfield(Symbol) const override;
    virtual void putfields(Symbol_Map<Value>&) const override;
    virtual Shared<List> fields() const override;
    virtual size_t size() const override { return fields_.size(); }
    virtual void each_field(std::function<void(Symbol,Value)>) const override;

    static const char name[];
};

} // namespace curv
#endif // header guard
