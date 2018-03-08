// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_LIST_H
#define CURV_LIST_H

#include <curv/value.h>
#include <curv/tail_array.h>
#include <curv/array_mixin.h>
#include <vector>

namespace curv {

struct Context;

struct List_Base : public Ref_Value
{
    List_Base() : Ref_Value(ty_list) {}
    virtual void print(std::ostream&) const;
    bool operator==(const List_Base&) const;
    void assert_size(size_t sz, const Context& cx) const;

    static const char name[];
    TAIL_ARRAY_MEMBERS(Value)
};

/// Representation of lists in the Curv runtime.
///
/// This is a variable length object: the size and the value array
/// are in the same object. This is very efficient for small lists:
/// only a single cache hit is required to access both the size and the data.
using List = Tail_Array<List_Base>;

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
