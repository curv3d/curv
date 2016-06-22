// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_PHRASE_H
#define CURV_PHRASE_H

#include <vector>
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
struct Phrase : public aux::Shared_Base
{
    virtual ~Phrase() {}
    virtual Location location() const = 0;
};

struct Identifier final : public Phrase
{
    Identifier(const Script& s, Token id)
    : loc_(s, std::move(id))
    {}
    Location loc_;
    virtual Location location() const { return loc_; }
};

struct Numeral final : public Phrase
{
    Numeral(const Script& s, Token num)
    : loc_(s, std::move(num))
    {}
    Location loc_;
    virtual Location location() const { return loc_; }
};

struct Unary_Phrase : public Phrase
{
    Unary_Phrase(Token op, aux::Shared_Ptr<Phrase> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    aux::Shared_Ptr<Phrase> arg_;
    virtual Location location() const
    {
        return arg_->location().starting_at(op_);
    }
};

struct Binary_Phrase : public Phrase
{
    Binary_Phrase(
        aux::Shared_Ptr<Phrase> left,
        Token op,
        aux::Shared_Ptr<Phrase> right)
    :
        left_(std::move(left)),
        op_(op),
        right_(std::move(right))
    {}
    aux::Shared_Ptr<Phrase> left_;
    Token op_;
    aux::Shared_Ptr<Phrase> right_;
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token());
    }
};

struct Definition : public Phrase
{
    Definition(
        aux::Shared_Ptr<Phrase> left,
        Token equate,
        aux::Shared_Ptr<Phrase> right)
    :
        left_(left), equate_(equate), right_(right)
    {}
    aux::Shared_Ptr<Phrase> left_;
    Token equate_;
    aux::Shared_Ptr<Phrase> right_;
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token());
    }
};

/// a parenthesized argument list, part of a function call
struct Paren_Phrase : public Phrase
{
    // an expression followed by an optional comma
    struct Arg
    {
        aux::Shared_Ptr<Phrase> expr_;
        Token comma_;

        Arg(aux::Shared_Ptr<Phrase> expr) : expr_(expr) {}
    };

    const Script& script_;
    Token lparen_;
    std::vector<Arg> args_;
    Token rparen_;

    Paren_Phrase(const Script& script, Token lparen)
    : script_(script), lparen_(lparen)
    {}

    virtual Location location() const
    {
        return Location(script_, lparen_).ending_at(rparen_);
    }
};

/// a function call
struct Call_Phrase : public Phrase
{
    aux::Shared_Ptr<Phrase> function_;
    aux::Shared_Ptr<Phrase> args_;

    Call_Phrase(
        aux::Shared_Ptr<Phrase> function,
        aux::Shared_Ptr<Phrase> args)
    :
        function_(function),
        args_(args)
    {}

    virtual Location location() const
    {
        return function_->location().ending_at(args_->location().token());
    }
};

} // namespace curv
#endif // header guard
