// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/type.h>

namespace curv {

Shared<const Type> Type::Bool = make<Bool_Type>();
Shared<const Type> Type::Bool32 = make<List_Type>(32, Type::Bool);
Shared<const Type> Type::Num = make<Num_Type>();

void Bool_Type::print_repr(std::ostream& out) const
{
    out << "Bool";
};

void Num_Type::print_repr(std::ostream& out) const
{
    out << "Num";
};

void List_Type::print_repr(std::ostream& out) const
{
    out << "List " << count_ << " (";
    elem_type_->print_repr(out);
    out << ")";
};

Plex_Type List_Type::make_plex_type(unsigned count, Shared<const Type> etype)
{
    if (etype->plex_type_ == Plex_Type::Bool) {
        if (count >= 2 && count <= 4)
            return Plex_Type(unsigned(Plex_Type::Bool2) + count-2);
        if (count == 32)
            return Plex_Type::Bool32;
    }
    else if (etype->plex_type_ == Plex_Type::Num) {
        if (count >= 2 && count <= 4)
            return Plex_Type(unsigned(Plex_Type::Vec2) + count-2);
    }
    else if (etype->plex_type_ == Plex_Type::Bool32) {
        if (count >= 2 && count <= 4)
            return Plex_Type(unsigned(Plex_Type::Bool2x32) + count-2);
    }
    else if (etype->plex_type_ >= Plex_Type::Vec2
             && etype->plex_type_ <= Plex_Type::Vec4)
    {
        unsigned c = unsigned(etype->plex_type_) - unsigned(Plex_Type::Vec2) +2;
        if (count == c)
            return Plex_Type(unsigned(Plex_Type::Mat2) + c - 2);
    }
    return Plex_Type::missing;
}

bool Type::equal(Shared<const Type> t1, Shared<const Type> t2)
{
    if (t1 && t2) {
        if (t1->subtype_ != t2->subtype_) return false;
        if (t1->subtype_ == Ref_Value::sty_list_type) {
            auto l1 = (List_Type*)(&*t1);
            auto l2 = (List_Type*)(&*t2);
            return l1->count_ == l2->count_
                && equal(l1->elem_type_,l2->elem_type_);
        }
        return true;
    }
    return t1 == t2;
}

} // namespace curv
