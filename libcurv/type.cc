// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/type.h>

#include <libcurv/bool.h>
#include <libcurv/exception.h>
#include <libcurv/num.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/symbol.h>
#include <vector>

namespace curv {

Shared<const Type> Type::Error = make<Error_Type>();
Shared<const Type> Type::Bool = make<Bool_Type>();
Shared<const Type> Type::Bool32 = make<List_Type>(32, Type::Bool);
Shared<const Type> Type::Num = make<Num_Type>();

const char* glsl_plex_type_name[] = {
    "Error",
    "bool",
    "bvec2",
    "bvec3",
    "bvec4",
    "uint",
    "uvec2",
    "uvec3",
    "uvec4",
    "float",
    "vec2",
    "vec3",
    "vec4",
    "mat2",
    "mat3",
    "mat4",
};

const char Type::name[] = "type";

Shared<const Type> value_to_type(Value v, Fail fl, const Context& cx)
{
    static Symbol_Ref Tkey = make_symbol("T");
    for (;;) {
        if (auto type = v.maybe<const Type>())
            return type;
        if (auto rec = v.maybe<const Record>()) {
            if (rec->hasfield(Tkey)) {
                v = rec->getfield(Tkey, cx);
                continue;
            }
        }
        FAIL(fl, nullptr, cx, stringify(v," is not a type"));
    }
}

bool Error_Type::contains(Value val, const Context&) const
{
    return false;
}
void Error_Type::print_repr(std::ostream& out) const
{
    out << "Error";
};

bool Bool_Type::contains(Value val, const Context&) const
{
    return is_bool(val);
}
void Bool_Type::print_repr(std::ostream& out) const
{
    out << "Bool";
};

bool Num_Type::contains(Value val, const Context&) const
{
    return is_num(val);
}
void Num_Type::print_repr(std::ostream& out) const
{
    out << "Num";
};

bool List_Type::contains(Value val, const Context& cx) const
{
    Generic_List list(val);
    if (!list.is_list()) return false;
    return list.has_elem_type(*elem_type_, cx);
}
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

bool Char_Type::contains(Value val, const Context&) const
{
    return val.is_char();
}
void Char_Type::print_repr(std::ostream& out) const
{
    out << "Char";
};

bool Any_Type::contains(Value val, const Context&) const
{
    return true;
}
void Any_Type::print_repr(std::ostream& out) const
{
    out << "Any";
};

bool Type_Type::contains(Value val, const Context& cx) const
{
    return value_to_type(val, Fail::soft, cx) != nullptr;
}
void Type_Type::print_repr(std::ostream& out) const
{
    out << "Type";
};

#if 0
bool Func_Type::contains(Value val, const Context&) const
{
    return maybe_function(val) != nullptr; // TODO need cx
}
void Func_Type::print_repr(std::ostream& out) const
{
    out << "Func";
};

bool Primitive_Func_Type::contains(Value val, const Context&) const
{
    return val.is_char();
}
void Primitive_Func_Type::print_repr(std::ostream& out) const
{
    out << "Primitive_Func";
};

bool Index_Type::contains(Value val, const Context&) const
{
    return val.is_char();
}
void Index_Type::print_repr(std::ostream& out) const
{
    out << "Index";
};

bool Symbol_Type::contains(Value val, const Context&) const
{
    return val.is_char();
}
void Symbol_Type::print_repr(std::ostream& out) const
{
    out << "Symbol";
};
#endif

bool Type::equal(const Type& t1, const Type& t2)
{
    if (t1.subtype_ != t2.subtype_) return false;
    if (t1.subtype_ == Ref_Value::sty_list_type) {
        auto l1 = (const List_Type*)(&t1);
        auto l2 = (const List_Type*)(&t2);
        return l1->count_ == l2->count_
            && equal(*l1->elem_type_, *l2->elem_type_);
    }
    return true;
}

unsigned Type::rank() const
{
    auto t = this;
    unsigned rank = 0;
    while (t->subtype_ == Ref_Value::sty_list_type)
    {
        t = &*((const List_Type*)(t))->elem_type_;
        ++rank;
    }
    return rank;
}

Shared<const Type> Type::plex_array_base() const
{
    auto t = this;
    while (t->subtype_ == Ref_Value::sty_list_type
           && t->plex_type_ == Plex_Type::missing)
    {
        t = &*((const List_Type*)(t))->elem_type_;
    }
    return share(*t);
}

unsigned Type::plex_array_rank() const
{
    auto t = this;
    unsigned rank = 0;
    while (t->subtype_ == Ref_Value::sty_list_type
           && t->plex_type_ == Plex_Type::missing)
    {
        t = &*((const List_Type*)(t))->elem_type_;
        ++rank;
    }
    return rank;
}

unsigned Type::plex_array_dim(unsigned i) const
{
    // This is expensive, but this function will go away once I am finished
    // refactoring the code that uses SC_Type.
    std::vector<unsigned> dims;
    auto t = this;
    while (t->subtype_ == Ref_Value::sty_list_type
           && t->plex_type_ == Plex_Type::missing)
    {
        auto li = (const List_Type*)(t);
        dims.push_back(li->count_);
        t = &*li->elem_type_;
    }
    return dims.at(i);
}

} // namespace curv
