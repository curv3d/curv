// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_LIST_H
#define CURV_LIST_H

#include <curv/value.h>
#include <aux/tail_array.h>
#include <aux/array_mixin.h>

namespace curv {

struct List_Base : public Ref_Value, public aux::Tail_Array_Data<Value>
{
    List_Base() : Ref_Value(ty_list) {}
    virtual void print(std::ostream&) const;
    bool operator==(const List_Base&) const;
};

/// Representation of lists in the Curv runtime.
///
/// This is a variable length object: the size and the value array
/// are in the same object. This is very efficient for small lists:
/// only a single cache hit is required to access both the size and the data.
using List = aux::Tail_Array<List_Base>;

} // namespace curv
#endif // header guard
