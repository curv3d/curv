// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TYPES_H
#define LIBCURV_TYPES_H

#include <libcurv/type.h>
#include <libcurv/symbol.h>
#include <vector>

namespace curv {

// the empty set, containing no values
struct Error_Type : public Type
{
    static constexpr int subtype = sty_error_type;
    Error_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

// Any : the set of all values
struct Any_Type : public Type
{
    static constexpr int subtype = sty_any_type;
    Any_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

// Type: the set of all type values
struct Type_Type : public Type
{
    static constexpr int subtype = sty_type_type;
    Type_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

// Bool: the set containing #true and #false
struct Bool_Type : public Type
{
    static constexpr int subtype = sty_bool_type;
    Bool_Type() : Type(subtype, Plex_Type::Bool) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Num_Type : public Type
{
    static constexpr int subtype = sty_num_type;
    Num_Type() : Type(subtype, Plex_Type::Num) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Char_Type : public Type
{
    static constexpr int subtype = sty_char_type;
    Char_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Func_Type : public Type
{
    static constexpr int subtype = sty_func_type;
    Func_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Symbol_Type : public Type
{
    static constexpr int subtype = sty_symbol_type;
    Symbol_Type() : Type(subtype, Plex_Type::missing) {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Tuple_Type : public Type
{
    std::vector<CType> elements_;
    static constexpr int subtype = sty_tuple_type;
    Tuple_Type(std::vector<CType> e)
      : Type(subtype, Plex_Type::missing),
        elements_(std::move(e))
        {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Array_Type : public Type
{
    unsigned count_;
    CType elem_type_;
    static constexpr int subtype = sty_array_type;
    Array_Type(unsigned c, CType et)
    :
        Type(subtype, make_plex_type(c, et)),
        count_(c),
        elem_type_(et)
    {}
    static Plex_Type make_plex_type(unsigned, CType);
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct List_Type : public Type
{
    CType elem_type_;
    static constexpr int subtype = sty_list_type;
    List_Type(CType et)
    :
        Type(subtype, Plex_Type::missing),
        elem_type_(et)
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Struct_Type : public Type
{
    Symbol_Map<CType> fields_;
    static constexpr int subtype = sty_struct_type;
    Struct_Type(Symbol_Map<CType> fields)
    :
        Type(subtype, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Record_Type : public Type
{
    Symbol_Map<CType> fields_;
    static constexpr int subtype = sty_record_type;
    Record_Type(Symbol_Map<CType> fields)
    :
        Type(subtype, Plex_Type::missing),
        fields_(std::move(fields))
    {}
    virtual bool contains(Value, const At_Syntax&) const;
    virtual void print_repr(std::ostream&, Prec) const override;
};

} // namespace curv
#endif // header guard
