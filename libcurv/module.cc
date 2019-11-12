// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/module.h>
#include <libcurv/function.h>
#include <libcurv/exception.h>

namespace curv {

const char Module_Base::name[] = "module";

void
Module_Base::print(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (auto i : *this) {
        if (!first) out << ",";
        first = false;
        out << i.first << ":";
        i.second.print(out);
    }
    out << "}";
}

Value
Module_Base::get(slot_t i) const
{
    Value val = array_[i];
    // A recursive function is represented by a Closure, whose nonlocals_
    // member is a Module that contains Lambda objects wherever there is a
    // recursive function reference. This code converts those Lambda objects
    // into proper Values. (This is a trick to avoid reference cycles in the
    // representation of function values, which would break reference counting.)
    if (val.is_ref()) {
        auto& ref = val.to_ref_unsafe();
        if (ref.type_ == Ref_Value::ty_lambda)
            return {make<Closure>((Lambda&)ref, *(Module*)this)};
    }
    return val;
}

Value
Module_Base::find_field(Symbol_Ref name, const Context& cx) const
{
    auto b = dictionary_->find(name);
    if (b != dictionary_->end())
        return get(b->second);
    return missing;
}

bool
Module_Base::hasfield(Symbol_Ref name) const
{
    auto b = dictionary_->find(name);
    return (b != dictionary_->end());
}

Shared<Record>
Module_Base::clone() const
{
    return Module::make_copy(&array_[0], size(), dictionary_);
}

Value*
Module_Base::ref_field(Symbol_Ref name, bool need_value, const Context& cx)
{
    auto b = dictionary_->find(name);
    // WARNING: array_[i] can be a Closure, which is not a proper value.
    if (b != dictionary_->end())
        return &array_[b->second];
    throw Exception(cx, stringify(Value{share(*this)},
        " has no field named ", name));
}

} // namespace curv
