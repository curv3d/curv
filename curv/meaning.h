// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_MEANING_H
#define CURV_MEANING_H

#include <aux/tail_array.h>
#include <curv/shared.h>
#include <curv/phrase.h>
#include <curv/value.h>
#include <curv/atom.h>
#include <curv/list.h>
#include <curv/module.h>

namespace curv {

class Frame; // evaluation context, defined in eval.h

/// An abstract base class representing a semantically analyzed Phrase.
struct Meaning : public aux::Shared_Base
{
    /// The original source code for this meaning, or nullptr in the case
    /// of the Bindable objects stored in the builtin namespace.
    ///
    /// When a builtin is looked up, we have to copy the Bindable
    /// to add a source reference, after which source_ is not null.
    /// See Bindable::copy_with_source.
    ///
    /// The syntax of the source code need not have any relation to the meaning
    /// class, due to compile time evaluation. And that's why we separate
    /// the Phrase tree from the Meaning tree.
    Shared<const Phrase> source_;

    Meaning(Shared<const Phrase> source) : source_(std::move(source)) {}
};

#if 0 // maybe later when I need this
/// A Bindable phrase denotes an entity that can be bound to a name.
///
/// But the entity might not be a run-time value, in which case the phrase
/// is not an Expression. Bindable is a supertype of Expression.
///
/// Bindable is needed to represent compile time entities that are function-like
/// (can be invoked using function call syntax) or namespace-like
/// (can use '.' notation to reference members), but which aren't
/// run-time values.
struct Bindable : public Meaning
{
    virtual Shared<Meaning> analyze_dot(const Identifier&) const;
    virtual Shared<Meaning> analyze_call(Range<Shared<const Phrase>*>) const;
    virtual Shared<Expression> to_expression() const;
    virtual Shared<Bindable> copy_with_source(const Phrase&) const;
};
#endif

/// An Expression is a phrase that denotes a value.
struct Expression : public Meaning //Bindable
{
    using Meaning::Meaning;

//  virtual Shared<Meaning> analyze_dot(const Identifier&) const override;
//  virtual Shared<Meaning> analyze_call(Range<Shared<const Phrase>*>) const override;
//  virtual Shared<Expression> to_expression() const override;

    virtual Value eval(Frame&) const = 0;
};

/// A Constant is an Expression whose value is known at compile time.
struct Constant : public Expression
{
    Value value_;

    Constant(Shared<const Phrase> source, Value v)
    : Expression(std::move(source)), value_(std::move(v))
    {}

    virtual Value eval(Frame&) const override;
};

struct Module_Ref : public Expression
{
    Atom atom_;

    Module_Ref(Shared<const Phrase> source, Atom a)
    : Expression(std::move(source)), atom_(a)
    {}

    virtual Value eval(Frame&) const override;
};

struct Dot_Expr : public Expression
{
    Shared<Expression> base_;
    Atom id_;

    Dot_Expr(Shared<const Phrase> source, Shared<Expression> base, Atom id)
    :
        Expression(std::move(source)),
        base_(std::move(base)),
        id_(std::move(id))
    {}

    virtual Value eval(Frame&) const override;
};

struct Call_Expr : public Expression
{
    Shared<Expression> fun_;
    Shared<const Phrase> argsource_;
    std::vector<Shared<Expression>> args_;

    Call_Expr(
        Shared<const Phrase> source,
        Shared<Expression> fun,
        Shared<const Phrase> argsource,
        std::vector<Shared<Expression>> args)
    :
        Expression(std::move(source)),
        fun_(std::move(fun)),
        argsource_(std::move(argsource)),
        args_(std::move(args))
    {}

    virtual Value eval(Frame&) const override;
};

struct Prefix_Expr : public Expression
{
    Token::Kind op_;
    Shared<Expression> arg_;

    Prefix_Expr(
        Shared<const Phrase> source,
        Token::Kind op,
        Shared<Expression> arg)
    :
        Expression(source),
        op_(op),
        arg_(std::move(arg))
    {}

    virtual Value eval(Frame&) const override;
};
struct Prefix_Expr_Base : public Expression
{
    Shared<Expression> arg_;

    Prefix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Expression> arg)
    :
        Expression(source),
        arg_(std::move(arg))
    {}
};
struct Not_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame&) const override;
};

struct Infix_Expr : public Expression
{
    Token::Kind op_;
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;

    Infix_Expr(
        Shared<const Phrase> source,
        Token::Kind op,
        Shared<Expression> arg1,
        Shared<Expression> arg2)
    :
        Expression(source),
        op_(op),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}

    virtual Value eval(Frame&) const override;
};
struct Infix_Expr_Base : public Expression
{
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;

    Infix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Expression> arg1,
        Shared<Expression> arg2)
    :
        Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}
};
struct Or_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct And_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Not_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Less_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Greater_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Less_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Greater_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Power_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct At_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};

struct List_Expr_Base : public Expression,
    public aux::Tail_Array_Data<Shared<const Expression>>
{
    List_Expr_Base(Shared<const Phrase> source)
    : Expression(std::move(source)) {}

    virtual Value eval(Frame&) const override;
    Shared<List> eval_list(Frame&) const;
};
using List_Expr = aux::Tail_Array<List_Expr_Base>;

struct Record_Expr : public Expression
{
    Atom_Map<Shared<const Expression>> fields_;

    Record_Expr(Shared<const Phrase> source) : Expression(source) {}

    virtual Value eval(Frame&) const override;
};

struct Module_Expr : public Expression
{
    Atom_Map<Shared<const Expression>> fields_;
    Shared<const List_Expr> elements_;

    Module_Expr(Shared<const Phrase> source) : Expression(source) {}

    virtual Value eval(Frame&) const override;
    Shared<Module> eval_module() const;
};

struct If_Expr : public Expression
{
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;
    Shared<Expression> arg3_;

    If_Expr(
        Shared<const Phrase> source,
        Shared<Expression> arg1,
        Shared<Expression> arg2,
        Shared<Expression> arg3)
    :
        Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3))
    {}

    virtual Value eval(Frame&) const override;
};

} // namespace curv
#endif // header guard
