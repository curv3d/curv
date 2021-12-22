// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/types.h>

#include <libcurv/bool.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/list.h>
#include <libcurv/num.h>
#include <libcurv/record.h>
#include <libcurv/symbol.h>
#include <vector>

namespace curv {

CType Type::Error() {
    static const auto type = makev<Error_Type>();
    return type;
}
CType Type::Bool() {
    static const auto type = makev<Bool_Type>();
    return type;
}
CType Type::Bool32() {
    static const auto type = makev<Array_Type>(32, Type::Bool());
    return type;
}
CType Type::Num() {
    static const auto type = makev<Num_Type>();
    return type;
}

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

CType CType::from_value(Value v, Fail fl, const At_Syntax& cx)
{
    static Symbol_Ref Tkey = make_symbol("T");
    for (;;) {
        if (auto type = v.maybe<const Type>())
            return {type};
        if (auto rec = v.maybe<const Record>()) {
            if (rec->hasfield(Tkey)) {
                v = rec->getfield(Tkey, cx);
                continue;
            }
        }
        FAIL(fl, nullptr, cx, stringify(v," is not a type"));
    }
}

bool Error_Type::contains(Value val, const At_Syntax&) const
{
    return false;
}
void Error_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Error";
};

bool Bool_Type::contains(Value val, const At_Syntax&) const
{
    return is_bool(val);
}
void Bool_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Bool";
};

bool Num_Type::contains(Value val, const At_Syntax&) const
{
    return is_num(val);
}
void Num_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Num";
};

bool Tuple_Type::contains(Value val, const At_Syntax& cx) const
{
    Generic_List list(val);
    if (!list.is_list()) return false;
    if (list.size() != elements_.size()) return false;
    for (unsigned i = 0; i < elements_.size(); ++i) {
        if (!elements_[i].contains(list.val_at(i,cx), cx)) return false;
    }
    return true;
}
void Tuple_Type::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    out << "Tuple[";
    bool at_start = true;
    for (auto e : elements_) {
        if (!at_start) out << ",";
        e.print_repr(out, Prec::item);
        at_start = false;
    }
    out << "]";
    close_paren(out, rprec, Prec::postfix);
}

bool Array_Type::contains(Value val, const At_Syntax& cx) const
{
    Generic_List list(val);
    if (!list.is_list()) return false;
    if (list.size() != count_) return false;
    return list.has_elem_type(*elem_type_, cx);
}
void Array_Type::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    out << "Array[" << count_;
    auto ety = elem_type_;
    for (;;) {
        if (auto aty = ety.cast<const Array_Type>()) {
            out << "," << aty->count_;
            ety = aty->elem_type_;
        } else {
            out << "]";
            ety.print_repr(out, Prec::primary);
            break;
        }
    }
    close_paren(out, rprec, Prec::postfix);
};

bool List_Type::contains(Value val, const At_Syntax& cx) const
{
    Generic_List list(val);
    if (!list.is_list()) return false;
    return list.has_elem_type(*elem_type_, cx);
}
void List_Type::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    out << "List ";
    elem_type_.print_repr(out, Prec::primary);
    close_paren(out, rprec, Prec::postfix);
};

bool Struct_Type::contains(Value val, const At_Syntax& cx) const
{
    auto rec = val.maybe<const Record>();
    if (rec == nullptr) return false;
    auto p = rec->iter();
    for (auto fld : fields_) {
        if (p->empty()) return false;
        if (p->key() != fld.first) return false;
        if (!fld.second.contains(p->value(cx), cx)) return false;
        p->next();
    }
    return p->empty();
}
void Struct_Type::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    out << "Struct {";
    bool at_start = true;
    for (auto fld : fields_) {
        if (!at_start) out << ", ";
        out << fld.first << ": " << fld.second;
        at_start = false;
    }
    out << "}";
    close_paren(out, rprec, Prec::postfix);
};

bool Record_Type::contains(Value val, const At_Syntax& cx) const
{
    auto rec = val.maybe<const Record>();
    if (rec == nullptr) return false;
    auto p = rec->iter();
    for (auto fld : fields_) {
        if (p->empty()) return false;
        while (p->key() < fld.first)
            p->next();
        if (p->key() != fld.first) return false;
        if (!fld.second.contains(p->value(cx), cx)) return false;
        p->next();
    }
    return true;
}
void Record_Type::print_repr(std::ostream& out, Prec rprec) const
{
    open_paren(out, rprec, Prec::postfix);
    out << "Record {";
    bool at_start = true;
    for (auto fld : fields_) {
        if (!at_start) out << ", ";
        out << fld.first << ": " << fld.second;
        at_start = false;
    }
    out << "}";
    close_paren(out, rprec, Prec::postfix);
};

Plex_Type Array_Type::make_plex_type(unsigned count, CType etype)
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

bool Char_Type::contains(Value val, const At_Syntax&) const
{
    return val.is_char();
}
void Char_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Char";
};

bool Any_Type::contains(Value val, const At_Syntax&) const
{
    return true;
}
void Any_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Any";
};

bool Type_Type::contains(Value val, const At_Syntax& cx) const
{
    return CType::from_value(val, Fail::soft, cx).has_value();
}
void Type_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Type";
};

bool Func_Type::contains(Value val, const At_Syntax& cx) const
{
    return maybe_function(val, cx) != nullptr;
}
void Func_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Func";
};

bool Symbol_Type::contains(Value val, const At_Syntax&) const
{
    return is_symbol(val);
}
void Symbol_Type::print_repr(std::ostream& out, Prec) const
{
    out << "Symbol";
};

bool Type::equal(const Type& t1, const Type& t2)
{
    if (t1.subtype_ != t2.subtype_) return false;
    if (t1.subtype_ == Ref_Value::sty_array_type) {
        auto l1 = (const Array_Type*)(&t1);
        auto l2 = (const Array_Type*)(&t2);
        return l1->count_ == l2->count_
            && l1->elem_type_ == l2->elem_type_;
    }
    if (t1.subtype_ == Ref_Value::sty_tuple_type) {
        auto tt1 = (const Tuple_Type*)(&t1);
        auto tt2 = (const Tuple_Type*)(&t2);
        if (tt1->elements_.size() != tt2->elements_.size())
            return false;
        for (unsigned i = 0; i < tt1->elements_.size(); ++i) {
            if (tt1->elements_[i] != tt2->elements_[i])
                return false;
        }
        return true;
    }
    if (t1.subtype_ == Ref_Value::sty_list_type) {
        auto l1 = (const List_Type*)(&t1);
        auto l2 = (const List_Type*)(&t2);
        return l1->elem_type_ == l2->elem_type_;
    }
    if (t1.subtype_ == Ref_Value::sty_struct_type) {
        auto st1 = (const Struct_Type*)(&t1);
        auto st2 = (const Struct_Type*)(&t2);
        return st1->fields_ == st2->fields_;
    }
    if (t1.subtype_ == Ref_Value::sty_record_type) {
        auto rt1 = (const Record_Type*)(&t1);
        auto rt2 = (const Record_Type*)(&t2);
        return rt1->fields_ == rt2->fields_;
    }
    return true;
}

unsigned Type::rank() const
{
    auto t = this;
    unsigned rank = 0;
    while (t->subtype_ == Ref_Value::sty_array_type)
    {
        t = &*((const Array_Type*)(t))->elem_type_;
        ++rank;
    }
    return rank;
}

CType Type::plex_array_base() const
{
    auto t = this;
    while (t->subtype_ == Ref_Value::sty_array_type
           && t->plex_type_ == Plex_Type::missing)
    {
        t = &*((const Array_Type*)(t))->elem_type_;
    }
    return sharev(*t);
}

unsigned Type::plex_array_rank() const
{
    auto t = this;
    unsigned rank = 0;
    while (t->subtype_ == Ref_Value::sty_array_type
           && t->plex_type_ == Plex_Type::missing)
    {
        t = &*((const Array_Type*)(t))->elem_type_;
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
    while (t->subtype_ == Ref_Value::sty_array_type
           && t->plex_type_ == Plex_Type::missing)
    {
        auto li = (const Array_Type*)(t);
        dims.push_back(li->count_);
        t = &*li->elem_type_;
    }
    return dims.at(i);
}

} // namespace curv
