// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LIST_H
#define LIBCURV_LIST_H

#include <libcurv/value.h>
#include <libcurv/tail_array.h>
#include <libcurv/alist.h>
#include <string>
#include <vector>

namespace curv {

struct At_Syntax;
struct Context;
struct List_Base;

// abstract list API (works for List and Reactive_Value)
bool is_list(Value a);
size_t list_count(Value);
Value list_elem(Value, size_t, const At_Syntax&);

/// Representation of lists in the Curv runtime.
///
/// This is a variable length object: the size and the value array
/// are in the same object. This is very efficient for small lists:
/// only a single cache hit is required to access both the size and the data.
using List = Tail_Array<List_Base>;

struct List_Base : public Abstract_List
{
    List_Base() : Abstract_List(ty_list) {}
    virtual Value val_at(size_t i) const override { return array_[i]; }
    virtual void print_repr(std::ostream&) const;
    Ternary equal(const List_Base&, const Context&) const;
    void assert_size(size_t sz, const Context& cx) const;

    Shared<List> clone() const;
    Value* ref_element(Value, bool need_value, const Context&);
    Value* ref_lens(Value, bool need_value, const Context&);

    static const char name[];
    TAIL_ARRAY_MEMBERS_MOD_SIZE(Value)
};

inline std::ostream&
operator<<(std::ostream& out, const List_Base& list)
{
    list.print_repr(out);
    return out;
}

inline Shared<List> make_list(size_t size)
{
    auto list = List::make(size);
    return {std::move(list)};
}

// Factory class for making a Curv Abstract_List value,
// constructing a curv::List or curv::String depending on data.
// Each Curv list value has a single canonical representation:
//  * an empty list is a curv::List
//  * a non-empty list containing only characters is a curv::String
//  * otherwise, a curv::List
//
// TODO: reserve_next(n) specifies how many elements are about to be added
// next, so that List_Builder can reserve space in the underlying collection.
// Calls reserve() on std::vector or std::string.
// See also size(), capacity(), reserve(new_capacity).
struct List_Builder
{
private:
    bool in_string_ = true;
    std::string string_;
    std::vector<Value> list_;
public:
    void push_back(Value);
    void concat(Value, const Context&);
    Value get_value();
};

} // namespace curv
#endif // header guard
