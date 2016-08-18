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
    Shared<const Expression> expr_;

    Thunk(Shared<const Expression> expr)
    :
        Ref_Value(ty_thunk),
        expr_(expr)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
