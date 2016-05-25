// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EXPR_H
#define CURV_EXPR_H

#include <memory>
#include <aux/shared.h>
#include <curv/token.h>

namespace curv {

/// Base class for a node in a syntax tree created by the parser.
///
/// Goals for the syntax tree data structure:
/// * Cheap to construct (no semantic analysis that could be delayed until
///   JIT compile time).
/// * Preserves the original source code in full, which means preserving all
///   of the original tokens. So a syntax tree can be used for any purpose,
///   including upgrading source code from an earlier version of the language
///   to a newer version.
struct Syntax : public aux::Shared_Base
{
    const Script& script_;
    Syntax(const Script& script) : script_(script) {}
    virtual ~Syntax() {}
};

struct Identifier : public Syntax
{
    Identifier(const Script& s, Token id)
    : Syntax(s), id_(std::move(id))
    {}
    Token id_;
};

struct Numeral : public Syntax
{
    Numeral(const Script& s, Token n)
    : Syntax(s), num_(std::move(n)) {}
    Token num_;
};

struct Unary_Expr : public Syntax
{
    Unary_Expr(const Script& s, Token op, aux::Shared_Ptr<Syntax> arg)
    : Syntax(s), op_(op), arg_(std::move(arg))
    {}
    Token op_;
    aux::Shared_Ptr<Syntax> arg_;
};

struct Binary_Expr : public Syntax
{
    Binary_Expr(
        const Script& s,
        Token op,
        aux::Shared_Ptr<Syntax> left,
        aux::Shared_Ptr<Syntax> right)
    : Syntax(s), op_(op), left_(std::move(left)), right_(std::move(right))
    {}
    Token op_;
    aux::Shared_Ptr<Syntax> left_;
    aux::Shared_Ptr<Syntax> right_;
};

struct Paren_Expr : public Syntax
{
    Paren_Expr(const Script& s, Token lp, aux::Shared_Ptr<Syntax> arg, Token rp)
    : Syntax(s), lparen_(lp), arg_(std::move(arg)), rparen_(rp)
    {}
    Token lparen_;
    aux::Shared_Ptr<Syntax> arg_;
    Token rparen_;
};

struct Definition : public Syntax
{
    Definition(
        const Script& s,
        aux::Shared_Ptr<Syntax> left,
        Token equate,
        aux::Shared_Ptr<Syntax> right)
    :
        Syntax(s), left_(left), equate_(equate), right_(right)
    {}
    aux::Shared_Ptr<Syntax> left_;
    Token equate_;
    aux::Shared_Ptr<Syntax> right_;
};

} // namespace curv
#endif // header guard
