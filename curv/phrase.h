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
class Environ;

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
    virtual Shared<Meaning> analyze(const Environ&) const = 0;
};

struct Identifier final : public Phrase
{
    Identifier(const Script& s, Token id)
    : loc_(s, std::move(id))
    {}
    Location loc_;
    Atom make_atom() const { return {loc_.range()}; }
    virtual Location location() const { return loc_; }
    virtual Shared<Meaning> analyze(const Environ&) const;
};

struct Numeral final : public Phrase
{
    Numeral(const Script& s, Token num)
    : loc_(s, std::move(num))
    {}
    Location loc_;
    virtual Location location() const { return loc_; }
    virtual Shared<Meaning> analyze(const Environ&) const;
};

struct Unary_Phrase : public Phrase
{
    Unary_Phrase(Token op, Shared<Phrase> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    Shared<Phrase> arg_;
    virtual Location location() const
    {
        return arg_->location().starting_at(op_);
    }
    virtual Shared<Meaning> analyze(const Environ&) const;
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
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyze(const Environ&) const;
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
    virtual Location location() const
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyze(const Environ&) const;
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

    virtual Location location() const
    {
        return Location(script_, lparen_).ending_at(rparen_);
    }
};

struct Paren_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(const Environ&) const;
};

struct List_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(const Environ&) const;
};

struct Record_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyze(const Environ&) const;
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

    virtual Location location() const
    {
        if (stmts_.empty())
            return Location(script_, end_);
        else
            return stmts_[0].stmt_->location().ending_at(end_);
    }

    virtual Shared<Meaning> analyze(const Environ&) const;
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

    virtual Location location() const
    {
        return function_->location().ending_at(args_->location().token());
    }
    virtual Shared<Meaning> analyze(const Environ&) const;
};

struct Dot_Phrase : public Phrase
{
    Shared<Phrase> left_;
    Token dot_;
    Shared<Identifier> id_;

    Dot_Phrase(
        Shared<Phrase> left,
        Token dot,
        Shared<Identifier> id)
    :
        left_(std::move(left)),
        dot_(dot),
        id_(std::move(id))
    {}

    virtual Location location() const
    {
        return left_->location().ending_at(id_->location().token());
    }
    virtual Shared<Meaning> analyze(const Environ&) const;
};

} // namespace curv
#endif // header guard
