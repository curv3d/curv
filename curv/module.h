// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_MODULE_H
#define CURV_MODULE_H

#include <curv/value.h>
#include <curv/atom.h>
#include <curv/shared.h>
#include <curv/list.h>

namespace curv {

class Module_Expr;

/// A module value contains a set of fields and a list of elements.
///
/// TODO: Are module expressions strictly or lazily evaluated?
/// I've wanted lazy evaluation as a performance optimization, especially for
/// large libraries where most components aren't used. However, for now,
/// strict evaluation is simpler.
struct Module : public Ref_Value 
{
    /// A Dictionary maps field names onto slot indexes.
    /// It has a reference count so that the same dictionary
    /// can be shared between multiple modules.
    ///
    /// TODO: This might be more efficient as a sorted array of field names.
    /// The index of the field name would be interpreted as the slot index.
    /// (Reimplementing `Atom_Map` using hash trees is another proposal.)
    struct Dictionary : public aux::Shared_Base, public Atom_Map<size_t>
    {
        Dictionary() : aux::Shared_Base(), Atom_Map<size_t>() {}
    };

    /// The `dictionary` maps field names onto slot indexes.
    ///
    /// The dictionary is constructed at compile time, and the same dictionary
    /// can be shared between multiple module values.
    Shared<Dictionary> dictionary_;

    /// The `slots` array contains field values. In the future, for submodules,
    /// it will also contain nonlocals.
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
    ///
    /// This is not tail array because it may also be referenced by a Closure.
    /// (Well, unless we change Closure to contain a Module reference.)
    Shared<List> slots_;

    /// The `elements` array contains module elements. The number of
    /// elements is, in general, determined at run time.
    Shared<List> elements_;

    Module()
    :
        Ref_Value(ty_module)
    {}

    Module(Shared<Dictionary> dictionary, Shared<List> values)
    :
        Ref_Value(ty_module),
        dictionary_(std::move(dictionary)),
        slots_(std::move(values)),
        elements_(List::make(0))
    {}

    friend class curv::Module_Expr;

    /// Fetch the value stored at slot index `i`.
    Value get(size_t i) const;

    // We provide a container interface for accessing the fields, like std::map.
    size_t size() const { return dictionary_->size(); }

    struct iterator
    {
        const Module& m_;
        Dictionary::iterator iter_;

        iterator(const Module& m, bool first)
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

    Shared<List> elements() const { return elements_; }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom) const override;
};

} // namespace curv
#endif // header guard
