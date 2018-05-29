// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/module.h>
#include <libcurv/function.h>

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

void
Module_Base::putfields(Atom_Map<Value>& out) const
{
    for (auto i : *this)
        out[i.first] = i.second;
}

Value
Module_Base::get(slot_t i) const
{
    Value val = array_[i];
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        if (ref.type_ == Ref_Value::ty_lambda)
            return {make<Closure>((Lambda&)ref, *(Module*)this)};
    }
    return val;
}

Value
Module_Base::getfield(Atom name, const Context& cx) const
{
    auto b = dictionary_->find(name);
    if (b != dictionary_->end())
        return get(b->second);
    return Structure::getfield(name, cx);
}

bool
Module_Base::hasfield(Atom name) const
{
    auto b = dictionary_->find(name);
    return (b != dictionary_->end());
}

Shared<List>
Module_Base::fields() const
{
    auto list = List::make(dictionary_->size());
    int i = 0;
    for (auto f : *dictionary_) {
        list->at(i) = f.first.to_value();
        ++i;
    }
    return {std::move(list)};
}

void
Module_Base::each_field(std::function<void(curv::Atom,curv::Value)> visitor) const
{
    for (auto f : *dictionary_)
        visitor(f.first, get(f.second));
}

} // namespace curv
