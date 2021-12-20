// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/record.h>
#include <libcurv/exception.h>

namespace curv {

const char Record::name[] = "record";

Value
Record::getfield(Symbol_Ref field, const Context& cx) const
{
    Value val = find_field(field, cx);
    if (val.is_missing())
        throw Exception(cx, stringify(*this," does not contain the field .",field));
    return val;
}

Ternary
Record::equal(const Record& rhs, const Context& cx) const
{
    if (this->size() != rhs.size())
        return Ternary::False;
    Ternary result = Ternary::True;
    for (auto i = iter(); !i->empty(); i->next()) {
        if (!rhs.hasfield(i->key()))
            return Ternary::False;
        Ternary ter = i->value(cx).equal(rhs.getfield(i->key(),cx),cx);
        if (ter == Ternary::False)
            return Ternary::False;
        result &= ter;
    }
    return result;
}

void
Record::each_field(
    const Context& cx, std::function<void(Symbol_Ref,Value)> visitor) const
{
    for (auto f = iter(); !f->empty(); f->next())
        visitor(f->key(), f->value(cx));
}

Shared<List>
Record::fields() const
{
    auto list = make_tail_array<List>(size());
    int i = 0;
    for (auto f = iter(); !f->empty(); f->next()) {
        list->at(i) = f->key().to_value();
        ++i;
    }
    return {move(list)};
}

std::pair<Symbol_Ref, Value>
value_to_variant(Value val, const Context& cx)
{
    auto sym = maybe_symbol(val);
    if (!sym.empty()) {
        return std::pair<Symbol_Ref,Value>{sym, missing};
    }
    Shared<Record> rec = val.maybe<Record>();
    if (rec && rec->size() == 1) {
        auto i = rec->iter();
        return std::pair<Symbol_Ref,Value>{i->key(), i->value(cx)};
    }
    throw Exception(cx, stringify(val, " is not a variant"));
}

void
DRecord::print_repr(std::ostream& out, Prec) const
{
    out << "{";
    bool first = true;
    for (auto i : fields_) {
        if (!first) out << ",";
        first = false;
        out << i.first << ":";
        i.second.print_repr(out, Prec::item);
    }
    out << "}";
}

Value
DRecord::find_field(Symbol_Ref name, const Context& cx) const
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return fp->second;
    return missing;
}

bool
DRecord::hasfield(Symbol_Ref name) const
{
    auto fp = fields_.find(name);
    return (fp != fields_.end());
}

Shared<Record>
DRecord::clone() const
{
    return make<DRecord>(fields_);
}

Value*
DRecord::ref_field(Symbol_Ref name, bool need_value, const Context& cx)
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return &fp->second;
    throw Exception(cx, stringify(Value{share(*this)},
        " has no field named ", name));
}

Shared<DRecord> update_drecord(Value arg, const Context& cx)
{
    // TODO: optimize: if arg is a drecord with usecount==1, make no copy
    auto arec = arg.to<Record>(cx);
    auto drec = make<DRecord>();
    arec->each_field(cx, [&](Symbol_Ref id, Value val) -> void {
        drec->fields_[id] = val;
    });
    return drec;
}

} // namespace curv
