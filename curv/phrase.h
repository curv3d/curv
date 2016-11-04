// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_PHRASE_H
#define CURV_PHRASE_H

#include <vector>
#include <memory>
#include <curv/shared.h>
#include <curv/location.h>
#include <curv/atom.h>

namespace curv {

class Meaning;
class Module_Expr;
class Environ;

/// Base class for a node in a syntax tree created by the parser.
///
/// Goals for the syntax tree data structure:
/// * Cheap to construct (small footprint, no semantic analysis that could be
///   delayed until JIT compile time).
/// * Can reconstruct its Location in the source code, for error reporting.
///   This doesn't need to be fast.
/// * Preserves the original source code in full, which means preserving all of
///   the original tokens and white space. So a syntax tree can be used for any
///   purpose, including upgrading source code from an earlier version of the
///   language to a newer version.
struct Phrase : public aux::Shared_Base
{
    virtual ~Phrase() {}
    virtual Location location() const = 0;
    virtual Shared<Meaning> analyze(Environ&) const = 0;
};

/// Abstract implementation base class for Phrase classes
/// that encapsulate a single token.
struct Token_Phrase : public Phrase
{
    Location loc_;
    Token_Phrase(const Script& s, Token tok) : loc_(s, std::move(tok)) {}
    virtual Location location() const override { return loc_; }
};
struct Identifier final : public Token_Phrase
{
    Atom atom_;

    Identifier(const Script& s, Token tok)
    :
        Token_Phrase(s, std::move(tok)),
        atom_{loc_.range()}
    {}

    virtual Shared<Meaning> analyze(Environ&) const override;
};
struct Numeral final : public Token_Phrase
{
    using Token_Phrase::Token_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};
struct String_Phrase final : public Token_Phrase
{
    using Token_Phrase::Token_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Unary_Phrase : public Phrase
{
    Unary_Phrase(Token op, Shared<Phrase> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    Shared<Phrase> arg_;
    virtual Location location() const override
    {
        return arg_->location().starting_at(op_);
    }
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Binary_Phrase : public Phrase
{
    Binary_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(std::move(left)),
        op_(op),
        right_(std::move(right))
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Lambda_Phrase : public Binary_Phrase
{
    /// Normally false, this is set to true prior to analysis
    /// if the lambda expression occurs in the definiens position
    /// of a definition, and if it is being compiled to support
    /// recursive definitions.
    bool recursive_ = false;

    using Binary_Phrase::Binary_Phrase;

    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Definition : public Phrase
{
    Definition(
        Shared<Phrase> left,
        Token equate,
        Shared<Phrase> right)
    :
        left_(left), equate_(equate), right_(right)
    {}
    Shared<Phrase> left_;
    Token equate_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyze(Environ&) const override;
};

/// common implementation for `(a,b,c)` and `[a,b,c]` phrases.
struct Delimited_Phrase : public Phrase
{
    // an expression followed by an optional comma
    struct Arg
    {
        Shared<Phrase> expr_;
        Token comma_;

        Arg(Shared<Phrase> expr) : expr_(expr) {}
    };

    const Script& script_;
    Token lparen_;
    std::vector<Arg> args_;
    Token rparen_;

    Delimited_Phrase(const Script& script, Token lparen)
    : script_(script), lparen_(lparen)
    {}

    virtual Location location() const override
    {
        return Location(script_, lparen_).ending_at(rparen_);
    }
};

struct Paren_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct List_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Record_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct Module_Phrase : public Phrase
{
    // a statement (definition or expression) followed by an optional semicolon
    struct Statement
    {
        Shared<Phrase> stmt_;
        Token semicolon_;

        Statement(Shared<Phrase> stmt) : stmt_(stmt) {}
    };

    const Script& script_;
    std::vector<Statement> stmts_;
    Token end_;

    Module_Phrase(const Script& script) : script_(script) {}

    virtual Location location() const override
    {
        if (stmts_.empty())
            return Location(script_, end_);
        else
            return stmts_[0].stmt_->location().ending_at(end_);
    }

    virtual Shared<Meaning> analyze(Environ&) const override;
    Shared<Module_Expr> analyze_module(Environ&) const;
};

/// a function call
struct Call_Phrase : public Phrase
{
    Shared<Phrase> function_;
    Shared<Phrase> args_;

    Call_Phrase(
        Shared<Phrase> function,
        Shared<Phrase> args)
    :
        function_(std::move(function)),
        args_(std::move(args))
    {}

    virtual Location location() const override
    {
        return function_->location().ending_at(args_->location().token());
    }
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct If_Phrase : public Phrase
{
    Token if_;
    Shared<Phrase> condition_;
    Shared<Phrase> then_expr_;
    Token else_;
    Shared<Phrase> else_expr_;

    If_Phrase(
        Token t_if,
        Shared<Phrase> condition,
        Shared<Phrase> then_expr,
        Token t_else,
        Shared<Phrase> else_expr)
    :
        if_(t_if),
        condition_(condition),
        then_expr_(then_expr),
        else_(t_else),
        else_expr_(else_expr)
    {}

    virtual Location location() const override
    {
        if (else_expr_ == nullptr)
            return then_expr_->location().starting_at(if_);
        else
            return else_expr_->location().starting_at(if_);
    }
    virtual Shared<Meaning> analyze(Environ&) const override;
};

/// This is a generic syntax scheme: keyword (args) body,
/// used by many OpenSCAD primitives, and adopted by Curv.
struct Control_Phrase : public Phrase
{
    Token keyword_;
    Shared<Paren_Phrase> args_;
    Shared<Phrase> body_;

    Control_Phrase(
        Token keyword,
        Shared<Paren_Phrase> args,
        Shared<Phrase> body)
    :
        keyword_(keyword),
        args_(args),
        body_(body)
    {}

    virtual Location location() const override
    {
        return body_->location().starting_at(keyword_);
    }
};

struct Let_Phrase : public Control_Phrase
{
    using Control_Phrase::Control_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

struct For_Phrase : public Control_Phrase
{
    using Control_Phrase::Control_Phrase;
    virtual Shared<Meaning> analyze(Environ&) const override;
};

} // namespace curv
#endif // header guard
