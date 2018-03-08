// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_MODULE_H
#define CURV_MODULE_H

#include <curv/structure.h>
#include <curv/atom.h>
#include <curv/shared.h>
#include <curv/list.h>
#include <curv/slot.h>

namespace curv {

/// A module value contains a set of name/value pairs, specified
/// using a set of mutually recursive definitions.
///
/// Each module value constructed from the same module literal shares the
/// same dictionary. Only the value list is different.
struct Module_Base : public Structure
{
    /// A Dictionary maps field names onto slot indexes.
    /// It has a reference count so that the same dictionary
    /// can be shared between multiple modules.
    ///
    /// TODO: This might be more efficient as a sorted array of field names.
    /// The index of the field name would be interpreted as the slot index.
    /// (Reimplementing `Atom_Map` using hash trees is another proposal.)
    struct Dictionary : public Shared_Base, public Atom_Map<slot_t>
    {
        Dictionary() : Shared_Base(), Atom_Map<slot_t>() {}
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
        Structure(ty_module),
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
        std::pair<Atom,Value> operator*()
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

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom, const Context&) const override;
    virtual bool hasfield(Atom) const override;
    virtual void putfields(Atom_Map<Value>&) const override;
    virtual size_t size() const override { return size_; }
    virtual Shared<List> fields() const override;
    virtual void each_field(std::function<void(Atom,Value)>) const override;

    static const char name[];

protected:
    // interface used by Tail_Array. Must be declared last.
    using value_type = Value;
    size_t size_;
    Value array_[0];
};

using Module = Tail_Array<Module_Base>;

} // namespace curv
#endif // header guard
