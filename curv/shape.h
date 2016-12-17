// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHAPE_H
#define CURV_SHAPE_H

#include <curv/record.h>

namespace curv {

// TODO: a more compact/efficient representation for Shape2D?
// Maybe use the same internal representation as Record, with a different
// type tag?
// TODO: Shape2D should be abstract, user defined Shape2D should be a subclass.

struct Shape2D : public Ref_Value
{
    Shared<const Record> record_;

    Shape2D(Shared<const Record> record)
    : Ref_Value(ty_shape2d), record_(std::move(record))
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom) const override;
};

} // namespace curv
#endif // header guard
