// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_THUNK_H
#define CURV_THUNK_H

#include <curv/value.h>
#include <curv/meaning.h>

namespace curv {

/// A lazy evaluation thunk. Currently, Module fields are lazily evaluated:
/// they are initialized with thunks, which are replaced by proper values
/// on the first reference.
struct Thunk : public Ref_Value
{
    Shared<const Operation> expr_;
    slot_t nslots_;

    Thunk(Shared<const Operation> expr, slot_t nslots)
    :
        Ref_Value(ty_thunk),
        expr_(expr),
        nslots_(nslots)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

/// Lazy evaluation of an identifier reference to a module or block binding.
/// If the slot contains a thunk instead of a proper value, then the thunk
/// is evaluated and the slot is updated.
Value force_ref(Module& nonlocal, slot_t, const Phrase& identifier, Frame& f);

/// Lazy evaluation of a definiens in a module or letrec construct.
/// If the slot contains a thunk instead of a proper value, then the thunk
/// is evaluated and the slot is updated.
void force(Module&, slot_t, Frame& f);

} // namespace curv
#endif // header guard
