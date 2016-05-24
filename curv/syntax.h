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
    virtual ~Syntax() {}
};

struct IdentExpr : public Syntax
{
    IdentExpr(Token id) : identifier(std::move(id)) {}
    Token identifier;
};

struct NumExpr : public Syntax
{
    NumExpr(Token n) : numeral(std::move(n)) {}
    Token numeral;
};

struct UnaryExpr : public Syntax
{
    UnaryExpr(Token op, aux::Shared_Ptr<Syntax> arg)
    : optor(op), argument(std::move(arg))
    {}
    Token optor;
    aux::Shared_Ptr<Syntax> argument;
};

struct BinaryExpr : public Syntax
{
    BinaryExpr(Token op, aux::Shared_Ptr<Syntax> l, aux::Shared_Ptr<Syntax> r)
    : optor(op), left(std::move(l)), right(std::move(r))
    {}
    Token optor;
    aux::Shared_Ptr<Syntax> left;
    aux::Shared_Ptr<Syntax> right;
};

struct ParenExpr : public Syntax
{
    ParenExpr(Token lp, aux::Shared_Ptr<Syntax> arg, Token rp)
    : lparen(lp), argument(std::move(arg)), rparen(rp)
    {}
    Token lparen;
    aux::Shared_Ptr<Syntax> argument;
    Token rparen;
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
};

} // namespace curv
#endif // header guard
