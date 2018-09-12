// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/dir_record.h>

namespace curv {

Value dir_importer(const Filesystem::path& dir, const Context& cx)
{
    return {make<Dir_Record>(dir, cx)};
}

/*
struct Dir_Record : public Record
{
    Filesystem::path dir_;
    struct File
    {
        System::Importer importer_;
        Value value_;
    };
    Symbol_Map<File> fields_;
};
*/

Dir_Record::Dir_Record(Filesystem::path dir, const Context& )
:
    Record(Ref_Value::sty_dir_record),
    dir_(dir),
    fields_{}
{
}

void Dir_Record::print(std::ostream& out) const
{
    out << "{dir}";
}

Value Dir_Record::getfield(Symbol, const Context&) const
{
    return {};
}

bool Dir_Record::hasfield(Symbol) const
{
    return false;
}

size_t Dir_Record::size() const
{
    return fields_.size();
}

/*
class Dir_Record::Iter : public Record::Iter
{
protected:
    Dir_Record& rec_;
    Symbol_Map<File>::const_iterator i_;
};
*/

Dir_Record::Iter::Iter(const Dir_Record& rec)
:
    rec_{rec},
    i_{rec.fields_.begin()}
{
    if (i_ != rec_.fields_.end())
        key_ = i_->first;
}

void Dir_Record::Iter::load_value(const Context&)
{
    value_ = {};
}

void Dir_Record::Iter::next()
{
    ++i_;
    if (i_ != rec_.fields_.end())
        key_ = i_->first;
    else
        key_ = Symbol();
}

std::unique_ptr<Record::Iter> Dir_Record::iter() const
{
    return std::make_unique<Dir_Record::Iter>(*this);
}

}
