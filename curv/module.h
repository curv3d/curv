// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_MODULE_H
#define CURV_MODULE_H

#include <curv/value.h>
#include <curv/atom.h>
#include <curv/shared.h>
#include <curv/list.h>

namespace curv {

/// A boxed static function.
struct Module : public Ref_Value
{
    Atom_Map<Value> fields_;
    Shared<List> elements_;

    Module()
    :
        Ref_Value(ty_module)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
