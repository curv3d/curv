// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PHRASE_H
#define LIBCURV_PHRASE_H

#include <vector>
#include <memory>
#include <libcurv/fail.h>
#include <libcurv/shared.h>
#include <libcurv/location.h>
#include <libcurv/symbol.h>
#include <libcurv/tail_array.h>

namespace curv {

struct Definition;
struct Meaning;
struct Operation;
struct Segment;
struct String_Expr;
struct Environ;
struct Interp;

/// Base class for a node in a syntax tree created by the parser.
///
/// Goals for the syntax tree data structure:
/// * Cheap to construct (small footprint, no semantic analysis that could be
///   delayed until JIT compile time).
/// * Can reconstruct its location in the source code, for error reporting.
///   This doesn't need to be fast.
/// * Preserves the original source code in full, which means preserving all of
///   the original tokens and white space. So a syntax tree can be used for any
///   purpose, including upgrading source code from an earlier version of the
///   language to a newer version.
struct Phrase : public Shared_Base
{
    virtual ~Phrase() {}
    virtual Src_Loc location() const = 0;
    virtual Shared<Definition> as_definition(Environ&, Fail) const;
    virtual Shared<Meaning> analyse(Environ&, Interp terp) const = 0;
};

/// Abstract implementation base class for Phrase classes
/// that encapsulate a single token.
struct Token_Phrase : public Phrase
{
    Src_Loc loc_;
    Token_Phrase(const Source& s, Token tok) : loc_(share(s), std::move(tok)) {}
    virtual Src_Loc location() const override { return loc_; }
};
struct Identifier final : public Token_Phrase
{
    Symbol_Ref symbol_;

    Identifier(const Source& s, Token tok)
    :
        Token_Phrase(s, std::move(tok)),
        symbol_{token_to_symbol(loc_.range())}
    {}

    bool quoted() const { return location().range()[0] == '\''; }

    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};
struct Numeral final : public Token_Phrase
{
    using Token_Phrase::Token_Phrase;
    Value eval() const;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Segment_Phrase : public Shared_Base
{
    virtual Src_Loc location() const = 0;
    virtual Shared<Segment> analyse(Environ&, Interp) const = 0;
};
struct String_Segment_Phrase : public Segment_Phrase
{
    Src_Loc loc_;
    String_Segment_Phrase(const Source& s, Token tok)
    : loc_(share(s), std::move(tok)) {}
    virtual Src_Loc location() const override { return loc_; }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct Char_Escape_Phrase : public Segment_Phrase
{
    Src_Loc loc_;
    Char_Escape_Phrase(const Source& s, Token tok)
    : loc_(share(s), std::move(tok)) {}
    virtual Src_Loc location() const override { return loc_; }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct Ident_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Ident_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Src_Loc location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct Paren_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Paren_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Src_Loc location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct Bracket_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Bracket_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Src_Loc location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct Brace_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Brace_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Src_Loc location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&, Interp) const override;
};
struct String_Phrase_Base : public Phrase
{
    Src_Loc begin_;
    Token end_;

    String_Phrase_Base(const Source& s, Token begin, Token end)
    :
        begin_{share(s), std::move(begin)},
        end_(std::move(end))
    {}

    virtual Src_Loc location() const override
    {
        return begin_.ending_at(end_);
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    Shared<String_Expr> analyse_string(Environ&) const;

    TAIL_ARRAY_MEMBERS(Shared<const Segment_Phrase>)
};
struct String_Phrase : public Tail_Array<String_Phrase_Base>
{
    using Tail_Array<String_Phrase_Base>::Tail_Array;
};

struct Unary_Phrase : public Phrase
{
    Unary_Phrase(Token op, Shared<Phrase> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    Shared<Phrase> arg_;
    virtual Src_Loc location() const override
    {
        return arg_->location().starting_at(op_);
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
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
    virtual Src_Loc location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    Range<const char*> opname() const
    {
        return left_->location().starting_at(op_).ending_at(op_).range();
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Local_Phrase : public Unary_Phrase
{
    using Unary_Phrase::Unary_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};
struct Include_Phrase : public Unary_Phrase
{
    using Unary_Phrase::Unary_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};
struct Test_Phrase : public Unary_Phrase
{
    using Unary_Phrase::Unary_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};

struct Dot_Phrase : public Binary_Phrase
{
    using Binary_Phrase::Binary_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};
struct Assoc_Phrase : public Binary_Phrase
{
    using Binary_Phrase::Binary_Phrase;
};

struct Lambda_Phrase : public Binary_Phrase
{
    // Normally false, this is set to true prior to analysis if the
    // lambda occurs in the definiens position of a recursive definition.
    // All of the function definitions in the same recursion group
    // share a single nonlocals object.
    bool shared_nonlocals_ = false;

    using Binary_Phrase::Binary_Phrase;

    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Where_Phrase : public Binary_Phrase
{
    using Binary_Phrase::Binary_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Recursive_Definition_Phrase : public Phrase
{
    Recursive_Definition_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(left), op_(op), right_(right)
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Src_Loc location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};
// This is 'var x := y'. DEPRECATED. Use 'local x = y' instead.
struct Var_Definition_Phrase : public Phrase
{
    Var_Definition_Phrase(
        Token var,
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        var_(var), left_(left), op_(op), right_(right)
    {}
    Token var_;
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Src_Loc location() const override
    {
        return right_->location().starting_at(var_);
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};
struct Assignment_Phrase : public Phrase
{
    Assignment_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(left), op_(op), right_(right)
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Src_Loc location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Empty_Phrase : public Phrase
{
    Src_Loc begin_;    // zero length location at start of phrase

    Empty_Phrase(Src_Loc begin) : begin_(std::move(begin)) {}

    virtual Src_Loc location() const override
    {
        return begin_;
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};

// Common implementation for Comma_Phrase and Semicolon_Phrase.
// In the surface grammar, a <listing> is a sequence of zero or more items
// separated by commas or semicolons, with an optional final comma or semi.
// However, a Listing_Phrase always contains at least one comma/semi.
struct Listing_Phrase : public Phrase
{
    // an expression followed by an optional separator
    struct Arg
    {
        Shared<Phrase> expr_;
        Token separator_;

        Arg(Shared<Phrase> expr, Token separator)
        : expr_(std::move(expr)), separator_(separator)
        {}
    };

    std::vector<Arg> args_;

    Listing_Phrase() {}

    virtual Src_Loc location() const override
    {
        const Arg& last = args_.back();
        return args_.front().expr_->location().ending_at(
            last.separator_.kind_ == Token::k_missing
            ? last.expr_->location().token()
            : last.separator_);
    }
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

/// `a,b,c` -- One or more items, separated by commas, with optional trailing
/// comma. Guaranteed to contain at least one comma token.
struct Comma_Phrase : public Listing_Phrase
{
    using Listing_Phrase::Listing_Phrase;
};

/// `a;b;c` -- One or more items, separated by semicolons, with optional
/// trailing semicolon. Guaranteed to contain at least one semicolon token.
struct Semicolon_Phrase : public Listing_Phrase
{
    using Listing_Phrase::Listing_Phrase;
};

/// common implementation for `(...)`, `[...]` and `{...}` phrases.
struct Delimited_Phrase : public Phrase
{
    Token lparen_;
    Shared<Phrase> body_;
    Token rparen_;

    Delimited_Phrase(Token lparen, Shared<Phrase> body, Token rparen)
    : lparen_(lparen), body_(std::move(body)), rparen_(rparen)
    {}

    virtual Src_Loc location() const override
    {
        return body_->location().starting_at(lparen_).ending_at(rparen_);
    }
};

struct Paren_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};

struct Bracket_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Brace_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Program_Phrase : public Phrase
{
    Shared<Phrase> body_;
    Token end_;

    Program_Phrase(Shared<Phrase> body, Token end)
    : body_(std::move(body)), end_(end)
    {}

    virtual Src_Loc location() const override
    {
        return body_->location().ending_at(end_);
    }

    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Shared<Definition> as_definition(Environ&, Fail) const override;
};

// This is the base class for phrases that may evaluate to a function call.
// If they do, a function call Frame is constructed and a reference to the
// call phrase is stored in Frame::call_phrase_.
//
// The internal interface is: the part of the phrase denoting the function
// is function_; the part denoting the function argument is arg_.
// The external interface is the free functions func_part() and arg_part().
//
// Some function call phrases are encoded in the base class, in which case
// op_.kind_ specifies which syntax was used (used by location()).
// Other function call phrases are defined as subclasses of Call_Phrase;
// eg, Predicate_Assertion_Phrase and Apply_Lens_Phrase.
struct Call_Phrase : public Phrase
{
    Shared<Phrase> function_;
    Token op_;  ///! k_missing or k_left_call or k_right_call or k_backtick
    Token op2_; ///! k_missing or k_backtick
    Shared<Phrase> arg_;

    Call_Phrase(
        Shared<Phrase> function,
        Shared<Phrase> arg,
        Token op = {},
        Token op2 = {})
    :
        function_(std::move(function)),
        op_(op),
        arg_(std::move(arg))
    {}

    // True if this is a juxtaposition phrase 'f x'.
    bool is_juxta() const
    {
        return op_.kind_ == Token::k_missing;
    }

    virtual Src_Loc location() const override
    {
        if (op_.kind_ == Token::k_backtick)
            return arg_->location();
        else if (op_.kind_ == Token::k_right_call)
            return arg_->location().ending_at(function_->location().token());
        else
            return function_->location().ending_at(arg_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Predicate_Assertion_Phrase : public Call_Phrase
{
    Predicate_Assertion_Phrase(
        Shared<Phrase> arg,
        Token op,
        Shared<Phrase> predicate)
    :
        Call_Phrase(std::move(predicate), std::move(arg), std::move(op))
    {}
    virtual Src_Loc location() const override
    {
        return arg_->location().ending_at(function_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Apply_Lens_Phrase : public Call_Phrase
{
    Apply_Lens_Phrase(
        Shared<Phrase> value,
        Token op,
        Shared<Phrase> lens)
    :
        Call_Phrase(std::move(lens), std::move(value), std::move(op))
    {}
    virtual Src_Loc location() const override
    {
        return arg_->location().ending_at(function_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Mutate_Phrase : public Call_Phrase
{
    Mutate_Phrase(
        Shared<Phrase> locative,
        Token op,
        Shared<Phrase> func)
    :
        Call_Phrase(std::move(func), std::move(locative), std::move(op))
    {}
    virtual Src_Loc location() const override
    {
        return arg_->location().ending_at(function_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
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

    virtual Src_Loc location() const override
    {
        if (else_expr_ == nullptr)
            return then_expr_->location().starting_at(if_);
        else
            return else_expr_->location().starting_at(if_);
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
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

    virtual Src_Loc location() const override
    {
        return body_->location().starting_at(keyword_);
    }
};

struct For_Phrase : public Phrase
{
    Token keyword_;
    Token lparen_;
    Shared<const Phrase> pattern_;
    Token in_;
    Shared<const Phrase> listexpr_;
    Token until_;
    Shared<const Phrase> condition_;
    Token rparen_;
    Shared<const Phrase> body_;

    For_Phrase(
        Token keyword,
        Token lparen,
        Shared<const Phrase> pattern,
        Token in,
        Shared<const Phrase> listexpr,
        Token Until,
        Shared<const Phrase> condition,
        Token rparen,
        Shared<const Phrase> body)
    :
        keyword_(keyword),
        lparen_(lparen),
        pattern_(pattern),
        in_(in),
        listexpr_(listexpr),
        until_(Until),
        condition_(condition),
        rparen_(rparen),
        body_(body)
    {}

    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Src_Loc location() const override
    {
        return body_->location().starting_at(keyword_);
    }
};

struct While_Phrase : public Control_Phrase
{
    using Control_Phrase::Control_Phrase;
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Parametric_Phrase : public Phrase
{
    Token keyword_;
    Shared<Brace_Phrase> param_;
    Shared<Phrase> body_;

    Parametric_Phrase(
        Token keyword,
        Shared<Brace_Phrase> param,
        Shared<Phrase> body)
    :
        keyword_(keyword),
        param_(param),
        body_(body)
    {}

    virtual Src_Loc location() const override
    {
        return body_->location().starting_at(keyword_);
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Range_Phrase : public Phrase
{
    Range_Phrase(
        Shared<Phrase> first,
        Token op1,
        Shared<Phrase> last,
        Token op2,
        Shared<Phrase> step)
    :
        first_(std::move(first)),
        op1_(op1),
        last_(std::move(last)),
        op2_(op2),
        step_(std::move(step))
    {}
    Shared<Phrase> first_;
    Token op1_;
    Shared<Phrase> last_;
    Token op2_;
    Shared<Phrase> step_;
    virtual Src_Loc location() const override
    {
        if (step_ != nullptr) {
            return first_->location().ending_at(step_->location().token());
        } else {
            return first_->location().ending_at(last_->location().token());
        }
    }
    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
};

struct Let_Phrase : public Phrase
{
    Token let_;
    Shared<Phrase> bindings_;
    Token in_;
    Shared<const Phrase> body_;

    Let_Phrase(
        Token let,
        Shared<Phrase> bindings,
        Token in,
        Shared<const Phrase> body)
    :
        let_(let),
        bindings_(bindings),
        in_(in),
        body_(body)
    {}

    virtual Shared<Meaning> analyse(Environ&, Interp) const override;
    virtual Src_Loc location() const override
    {
        return body_->location().starting_at(let_);
    }
};

// In the grammar, a <list> phrase is zero or more constituent phrases
// separated by commas or semicolons.
// This function iterates over each constituent phrase.
void each_item(Phrase& phrase, std::function<void(Phrase&)> func);

// Parenthesizing a phrase should not change its meaning.
// If the analyser uses RTTI to directly recognize and analyse specific
// phrase types, then it should call strip_parens() on the phrase first.
Shared<const Phrase> strip_parens(Shared<const Phrase>);

// Strip away let clauses, where clauses and redundant parentheses.
// Reduce the phrase to a smaller phrase with the same meaning, for use
// in error messages.
Shared<const Phrase> nub_phrase(Shared<const Phrase>);

// If ph is a Call_Phrase, return the function part, otherwise return ph.
Shared<const Phrase> func_part(Shared<const Phrase> ph);

// If ph is a Call_Phrase, return the argument part, otherwise return ph.
Shared<const Phrase> arg_part(Shared<const Phrase> ph);

} // namespace curv
#endif // header guard
