// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EXPR_H
#define CURV_EXPR_H

#include <memory>
#include <aux/shared.h>
#include <curv/location.h>

namespace curv {

/// Base class for a node in a syntax tree created by the parser.
///
/// Goals for the syntax tree data structure:
/// * Cheap to construct (small footprint, no semantic analysis that could be
///   delayed until JIT compile time).
/// * Can reconstruct its Location in the source code, for error reporting.
///   This doesn't need to be fast.
/// * Preserves the original source code in full, which means preserving all
///   of the original tokens. So a syntax tree can be used for any purpose,
///   including upgrading source code from an earlier version of the language
///   to a newer version.
struct Syntax : public aux::Shared_Base
{
    virtual ~Syntax() {}
    virtual Location location() const = 0;
};

struct Identifier final : public Syntax
{
    Identifier(const Script& s, Token id)
    : loc_(s, std::move(id))
    {}
    Location loc_;
    virtual Location location() const { return loc_; }
};

struct Numeral final : public Syntax
{
    Numeral(const Script& s, Token num)
    : loc_(s, std::move(num))
    {}
    Location loc_;
    virtual Location location() const { return loc_; }
};

struct Unary_Expr : public Syntax
{
    Unary_Expr(Token op, aux::Shared_Ptr<Syntax> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    aux::Shared_Ptr<Syntax> arg_;
    virtual Location location() const
    {
        return arg_->location().starting_at(op_);
    }
};

struct Binary_Expr : public Syntax
{
    Binary_Expr(
        aux::Shared_Ptr<Syntax> left,
        Token op,
        aux::Shared_Ptr<Syntax> right)
    :
        left_(std::move(left)),
        op_(op),
        right_(std::move(right))
    {}
    aux::Shared_Ptr<Syntax> left_;
    Token op_;
    aux::Shared_Ptr<Syntax> right_;
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token_);
    }
};

struct Paren_Expr : public Syntax
{
    Paren_Expr(Token lp, aux::Shared_Ptr<Syntax> arg, Token rp)
    : lparen_(lp), arg_(std::move(arg)), rparen_(rp)
    {}
    Token lparen_;
    aux::Shared_Ptr<Syntax> arg_;
    Token rparen_;
    virtual Location location() const
    {
        return arg_->location().starting_at(lparen_).ending_at(rparen_);
    }
};

struct Definition : public Syntax
{
    Definition(
        aux::Shared_Ptr<Syntax> left,
        Token equate,
        aux::Shared_Ptr<Syntax> right)
    :
        left_(left), equate_(equate), right_(right)
    {}
    aux::Shared_Ptr<Syntax> left_;
    Token equate_;
    aux::Shared_Ptr<Syntax> right_;
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token_);
    }
};

} // namespace curv
#endif // header guard
