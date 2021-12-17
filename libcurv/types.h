// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TYPES_H
#define LIBCURV_TYPES_H

#include <libcurv/type.h>
#include <libcurv/symbol.h>
#include <libcurv/context.h>
#include <vector>

namespace curv {

// This implements the concept "concrete value class" for types.
struct CType
{
    Shared<const Type> data_;
    CType() : data_(nullptr) {}
    CType(Shared<const Type> data) : data_(std::move(data)) {}
    CType(Value v, const At_Syntax& cx) : data_(v.to<const Type>(cx)) {}
    static CType from_value(Value v, const At_Syntax&)
      { return {v.maybe<const Type>()}; }
    static CType from_value(Value v, Fail fl, const At_Syntax&cx)
      { return {v.to<const Type>(fl,cx)}; }
    Value to_value() const { return Value{data_}; }
    bool operator==(const CType& t) const
      { return Type::equal(*data_, *t.data_); }
    void print_repr(std::ostream& o) { data_->print_repr(o); }
    void print_string(std::ostream& o) { data_->print_string(o); }
    explicit operator bool() const noexcept { return data_ != nullptr; }

    bool contains(Value v, const At_Syntax& cx) const
      { return data_->contains(v, cx); }
};
inline std::ostream& operator<<(std::ostream& o, CType t)
  { t.print_repr(o); return o; }

// the empty set, containing no values
struct Error_Type : public Type
{
    Error_Type() : Type(sty_error_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

// Any : the set of all values
struct Any_Type : public Type
{
    Any_Type() : Type(sty_any_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

// Type: the set of all type values
struct Type_Type : public Type
{
    Type_Type() : Type(sty_type_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

// Bool: the set containing #true and #false
struct Bool_Type : public Type
{
    Bool_Type() : Type(sty_bool_type, Plex_Type::Bool) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Num_Type : public Type
{
    Num_Type() : Type(sty_num_type, Plex_Type::Num) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Char_Type : public Type
{
    Char_Type() : Type(sty_char_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Func_Type : public Type
{
    Func_Type() : Type(sty_func_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Symbol_Type : public Type
{
    Symbol_Type() : Type(sty_symbol_type, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Tuple_Type : public Type
{
    std::vector<Shared<const Type>> elements_;
    Tuple_Type(std::vector<Shared<const Type>> e)
      : Type(sty_tuple_type, Plex_Type::missing),
        elements_(std::move(e))
        {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Array_Type : public Type
{
    unsigned count_;
    Shared<const Type> elem_type_;
    Array_Type(unsigned c, Shared<const Type> et)
    :
        Type(sty_array_type, make_plex_type(c, et)),
        count_(c),
        elem_type_(et)
    {}
    static Plex_Type make_plex_type(unsigned, Shared<const Type>);
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct List_Type : public Type
{
    Shared<const Type> elem_type_;
    List_Type(Shared<const Type> et)
    :
        Type(sty_list_type, Plex_Type::missing),
        elem_type_(et)
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Struct_Type : public Type
{
    Symbol_Map<CType> fields_;
    Struct_Type(Symbol_Map<CType> fields)
    :
        Type(sty_struct_type, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Record_Type : public Type
{
    Symbol_Map<CType> fields_;
    Record_Type(Symbol_Map<CType> fields)
    :
        Type(sty_struct_type, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

} // namespace curv
#endif // header guard
