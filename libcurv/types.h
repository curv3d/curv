// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TYPES_H
#define LIBCURV_TYPES_H

#include <libcurv/type.h>
#include <libcurv/symbol.h>
#include <vector>

namespace curv {
struct At_Syntax;

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
    Symbol_Map<Shared<const Type>> fields_;
    Struct_Type(Symbol_Map<Shared<const Type>> fields)
    :
        Type(sty_struct_type, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

struct Record_Type : public Type
{
    Symbol_Map<Shared<const Type>> fields_;
    Record_Type(Symbol_Map<Shared<const Type>> fields)
    :
        Type(sty_struct_type, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&) const override;
};

} // namespace curv
#endif // header guard
