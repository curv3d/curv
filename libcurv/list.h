// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LIST_H
#define LIBCURV_LIST_H

#include <libcurv/value.h>
#include <libcurv/tail_array.h>
#include <libcurv/alist.h>
#include <libcurv/string.h>
#include <string>
#include <vector>

namespace curv {

struct At_Syntax;
struct Context;
struct List_Base;
struct Reactive_Value;

/// List of boxed values.
///
/// This is a variable length object: the size and the value array
/// are in the same object. This is very efficient for small lists:
/// only a single cache hit is required to access both the size and the data.
using List = Tail_Array<List_Base>;

struct List_Base : public Abstract_List
{
    List_Base() : Abstract_List(sty_list) {}
    virtual Value val_at(size_t i) const override { return array_[i]; }
    virtual void print_repr(std::ostream&) const override;
    virtual void print_string(std::ostream&) const override;
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
    return {move(list)};
}

// A Generic_List is any Curv value that denotes a sequence of values.
// Lists have multiple representations, and Generic_List abstracts over
// all of those representations.
//
// The goal of this interface is to be abstract, efficient, convenient. Ideas:
// * Make it a template class. The Elem argument describes the supported
//   element types. It is used by the constructor to filter out unsupported
//   list representations and is used by operations at compile time to disable
//   conditional logic that isn't required under the element type restriction.
// * Provide bulk operations so that conditional logic can be performed once
//   per list rather than once per element.
struct Generic_List
{
private:
    // Either an Abstract_List, or a Reactive_Value of type list, or nullptr,
    // guaranteed by construction.
    Shared<Ref_Value> list_;
    bool is_abstract_list() const {
        return list_->type_ == Ref_Value::ty_abstract_list;
    }
    bool is_boxed_list() const {
        return list_->subtype_ == Ref_Value::sty_list;
    }
    bool is_string() const {
        return list_->subtype_ == Ref_Value::sty_string;
    }
    bool is_reactive_value() const {
        return list_->type_ == Ref_Value::ty_reactive;
    }
    Abstract_List& get_abstract_list() const {
        return *(Abstract_List*)(&*list_);
    }
    List& get_boxed_list() {
        return *(List*)(&*list_);
    }
    String& get_string() {
        return *(String*)(&*list_);
    }
    Reactive_Value& get_reactive_value() const {
        return *(Reactive_Value*)(&*list_);
    }

public:
    // If Value argument is not a list, then:
    // * Throw an exception on Fail::hard.
    // * Assert is_list() to be false on Fail::soft.
    Generic_List(Value, Fail, const Context& cx);

    // Test after construction, before invoking any list operations.
    bool is_list() const noexcept { return list_ != nullptr; }

    // List operations assume is_list().
    size_t size() const noexcept;
    void assert_size(size_t sz, const Context& cx) const;
    Value val_at(size_t i, const At_Syntax&) const;
    void amend_at(size_t, Value, const At_Syntax&);
    inline Value get_value() { return {list_}; }
};

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
