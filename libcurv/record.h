// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_RECORD_H
#define LIBCURV_RECORD_H

#include <libcurv/list.h>
#include <libcurv/symbol.h>

namespace curv {

// A curv 'Record' is a value containing a set of fields (name/value pairs).
// Subtypes are DRecord and Module.
// All records have the same protocol for field access, which is encapsulated
// in this pure virtual class.
struct Record : public Ref_Value
{
    Record(int subtype) : Ref_Value(ty_record, subtype) {}

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

    bool equal(const Record&, const Context&) const;

    static const char name[];

    // Each Record subclass defines a subclass of Iter.
    // The iter() function allocates an instance of this Iter subclass,
    // which initially either points to the first key/value pair in the record,
    // or is empty() if the Record is empty.
    class Iter
    {
    protected:
        Symbol key_{};
        Value value_{};
        virtual void load_value(const Context&) = 0;
    public:
        virtual ~Iter() {}
        bool empty() { return key_.empty(); }
        Symbol key() { return key_; }
        Value value(const Context& cx) {
            if (value_.eq(missing)) load_value(cx);
            return value_;
        }
        virtual void next() = 0;
    };
    virtual std::unique_ptr<Iter> iter() const = 0;
};

/// A DRecord is a dynamic record. It's a concrete implementation of the
/// Record protocol for which it is possible to dynamically add new fields
/// at run-time. Constrast this with Module, which is a static record.
struct DRecord : public Record
{
    Symbol_Map<Value> fields_;

    DRecord() : Record(sty_drecord) {}
    DRecord(Symbol_Map<Value> fields)
    :
        Record(sty_drecord),
        fields_(std::move(fields))
    {
    }

    Shared<const DRecord> clone() const
    {
        return make<const DRecord>(fields_);
    }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;
    virtual Value getfield(Symbol, const Context&) const override;
    virtual bool hasfield(Symbol) const override;
    virtual void putfields(Symbol_Map<Value>&) const override;
    virtual Shared<List> fields() const override;
    virtual size_t size() const override { return fields_.size(); }
    virtual void each_field(std::function<void(Symbol,Value)>) const override;

    class Iter : public Record::Iter
    {
    public:
        Iter(const DRecord& rec)
        :
            rec_(rec),
            i_(rec.fields_.begin())
        {
            if (i_ != rec_.fields_.end()) {
                key_ = i_->first;
                value_ = i_->second;
            }
        }
    protected:
        const DRecord& rec_;
        Symbol_Map<Value>::const_iterator i_;
        virtual void load_value(const Context&) override {}
        virtual void next() override
        {
            ++i_;
            if (i_ != rec_.fields_.end()) {
                key_ = i_->first;
                value_ = i_->second;
            } else
                key_ = Symbol();
        }
    };
    virtual std::unique_ptr<Record::Iter> iter() const override
    {
        return std::make_unique<Iter>(*this);
    }
};

} // namespace curv
#endif // header guard
