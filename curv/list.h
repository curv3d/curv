// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_LIST_H
#define CURV_LIST_H

#include <curv/value.h>
#include <aux/tail_array.h>
#include <aux/array_mixin.h>
#include <vector>

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

inline Shared<List> make_list(size_t size)
{
    auto list = List::make(size);
    return {std::move(list)};
}

/// Factory class for building a curv::List.
struct List_Builder : public std::vector<Value>
{
    // An optimized version of this class would use a curv::List
    // as the internal buffer.

    Shared<List> get_list();
};

} // namespace curv
#endif // header guard
