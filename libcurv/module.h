// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_MODULE_H
#define LIBCURV_MODULE_H

#include <libcurv/record.h>
#include <libcurv/symbol.h>
#include <libcurv/shared.h>
#include <libcurv/list.h>
#include <libcurv/slot.h>

namespace curv {

/// A module value contains a set of name/value pairs, specified
/// using a set of mutually recursive definitions.
///
/// Each module value constructed from the same module literal shares the
/// same dictionary. Only the value list is different.
struct Module_Base : public Record
{
    /// A Dictionary maps field names onto slot indexes.
    /// It has a reference count so that the same dictionary
    /// can be shared between multiple modules.
    ///
    /// TODO: This might be more efficient as a sorted array of field names.
    /// The index of the field name would be interpreted as the slot index.
    /// (Reimplementing `Symbol_Map` using hash trees is another proposal.)
    struct Dictionary : public Shared_Base, public Symbol_Map<slot_t>
    {
        Dictionary() : Shared_Base(), Symbol_Map<slot_t>() {}
    };

    /// The `dictionary` maps field names onto slot indexes.
    ///
    /// The dictionary is constructed at compile time, and the same dictionary
    /// can be shared between multiple module values.
    Shared<Dictionary> dictionary_;

    /// `array_` (declared at end) is the slots array,
    // containing field values and nonlocals.
    ///
    /// Field values that come from lambda expressions are represented in the
    /// slot array as Lambdas, not as Closures. (Otherwise, there would be a
    /// reference cycle from slots_ -> closure -> slots_, causing a storage
    /// leak.) To avoid the storage leak, we construct a Closure value each
    /// time such a slot is referenced via the `get` function.
    /// 
    /// The number of slots is determined at compile time, and slot indexes are
    /// determined at compile time. Which slots contain Lambdas is also known
    /// at compile time.

    Module_Base(Shared<Dictionary> dictionary)
    :
        Record(sty_module),
        dictionary_(std::move(dictionary))
    {}

    /// Fetch the contents of slot index `i`, normalize to a proper Value.
    Value get(slot_t i) const;

    Value& at(slot_t i) { return array_[i]; }

    // We provide a container interface for accessing the fields, like std::map.
    struct iterator
    {
        const Module_Base& m_;
        Dictionary::iterator iter_;

        iterator(const Module_Base& m, bool first)
        :
            m_(m),
            iter_(first ? m.dictionary_->begin() : m.dictionary_->end())
        {
        }
        std::pair<Symbol_Ref,Value> operator*()
        {
            return { iter_->first, m_.get(iter_->second) };
        }
        bool operator==(const iterator& i)
        {
            return iter_ == i.iter_;
        }
        bool operator!=(const iterator& i)
        {
            return iter_ != i.iter_;
        }
        void operator++()
        {
            ++iter_;
        }
    };
    iterator begin() const { return iterator(*this, true); }
    iterator end() const { return iterator(*this, false); }

    virtual void print_repr(std::ostream&) const override;

    virtual Value find_field(Symbol_Ref, const Context&) const override;
    virtual bool hasfield(Symbol_Ref) const override;
    virtual size_t size() const override { return size_; }
    virtual Shared<Record> clone() const override;
    virtual Value* ref_field(Symbol_Ref, bool need_value, const Context&) override;

    static const char name[];

    class Iter : public Record::Iter
    {
    public:
        Iter(const Module_Base& rec)
        :
            rec_(rec),
            i_(rec.dictionary_->begin())
        {
            if (i_ != rec_.dictionary_->end()) {
                key_ = i_->first;
                value_ = rec_.get(i_->second);
            }
        }
    protected:
        const Module_Base& rec_;
        Dictionary::const_iterator i_;
        virtual void load_value(const Context&) override {}
        virtual void next() override
        {
            ++i_;
            if (i_ != rec_.dictionary_->end()) {
                key_ = i_->first;
                value_ = rec_.get(i_->second);
            } else
                key_ = Symbol_Ref();
        }
    };
    virtual std::unique_ptr<Record::Iter> iter() const override
    {
        return std::make_unique<Iter>(*this);
    }

protected:
    // interface used by Tail_Array. Must be declared last.
    using value_type = Value;
    size_t size_;
    Value array_[0];
};

using Module = Tail_Array<Module_Base>;

} // namespace curv
#endif // header guard
