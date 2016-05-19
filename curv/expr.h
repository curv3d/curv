// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EXPR_H
#define CURV_EXPR_H

#include <memory>
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
struct Expr
{
    virtual ~Expr() {}
};

struct IdentExpr : public Expr
{
    IdentExpr(Token id) : identifier(std::move(id)) {}
    Token identifier;
};

struct NumExpr : public Expr
{
    NumExpr(Token n) : numeral(std::move(n)) {}
    Token numeral;
};

struct UnaryExpr : public Expr
{
    UnaryExpr(Token op, std::unique_ptr<Expr> arg)
    : optor(op), argument(std::move(arg))
    {}
    Token optor;
    std::unique_ptr<Expr> argument;
};

struct BinaryExpr : public Expr
{
    BinaryExpr(Token op, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
    : optor(op), left(std::move(l)), right(std::move(r))
    {}
    Token optor;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct ParenExpr : public Expr
{
    ParenExpr(Token lp, std::unique_ptr<Expr> arg, Token rp)
    : lparen(lp), argument(std::move(arg)), rparen(rp)
    {}
    Token lparen;
    std::unique_ptr<Expr> argument;
    Token rparen;
};

} // namespace curv
#endif // header guard
