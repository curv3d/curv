// Copyright Doug Moen 2016.
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

    /// The `slots` array contains field values, and may in future contain
    /// additional values. In any case, the number of slots is determined
    /// at compile time, and slot indexes are determined at compile time.
    /// TODO: tail array?
    Shared<List> slots_;

    /// The `elements` array contains module elements. The number of
    /// elements is, in general, determined at run time.
    Shared<List> elements_;

    Module()
    :
        Ref_Value(ty_module)
    {}

    friend class curv::Module_Expr;

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
            return { iter_->first, (*m_.slots_)[iter_->second] };
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

    Value operator[](Atom id) const
    {
        return (*slots_)[(*dictionary_)[id]];
    }

    Shared<List> elements() const { return elements_; }

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
