// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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

    Thunk(Shared<const Operation> expr)
    :
        Ref_Value(ty_thunk),
        expr_(expr)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

/// Lazy evaluation of an identifier reference to a module or let binding.
/// If the slot contains a thunk instead of a proper value, then the thunk
/// is evaluated and the slot is updated.
Value force_ref(Value& slot, const Phrase& identifier, Frame& f);

/// Lazy evaluation of a definiens in a module or let construct.
/// If the slot contains a thunk instead of a proper value, then the thunk
/// is evaluated and the slot is updated.
void force(Value& slot, Frame& f);

} // namespace curv
#endif // header guard
