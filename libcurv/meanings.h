// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_MEANINGS_H
#define LIBCURV_MEANINGS_H

#include <libcurv/function.h>
#include <libcurv/list.h>
#include <libcurv/meaning.h>
#include <libcurv/pattern.h>
#include <libcurv/record.h>
#include <libcurv/tail_array.h>
#include <vector>

namespace curv {

// Execute statements in a context like a `do` expression,
// where only pure actions are permitted.
struct Action_Executor : public Operation::Executor
{
    virtual void push_value(Value, const Context&) override;
    virtual void push_field(Symbol_Ref, Value, const Context&) override;
};
// Execute statements within a list comprehension.
struct List_Executor : public Operation::Executor
{
    List_Builder& list_;
    List_Executor(List_Builder& list) : list_(list) {}
    virtual void push_value(Value, const Context&) override;
    virtual void push_field(Symbol_Ref, Value, const Context&) override;
};
// Execute statements within a record comprehension.
struct Record_Executor : public Operation::Executor
{
    DRecord& record_;
    Record_Executor(DRecord& rec) : record_(rec) {}
    virtual void push_value(Value, const Context&) override;
    virtual void push_field(Symbol_Ref, Value, const Context&) override;
};

// `Just_Expression` is an implementation class, inherited by Operation
// classes whose instances are always expressions. It provides a default
// for `is_expr_` and the `exec` virtual function.
//
// An expression is an Operation that can be evaluated to produce a single 
// value. The work is done by the `eval` method, which must be defined.
// All expressions are also value generators that produce a single value,
// so the `exec` function calls `eval`.
//
// This is not an interface class, and not all expression objects are derived
// from Just_Expression. Functions should not take Just_Expressions as values
// or return Just_Expressions as results: use Operation instead.
struct Just_Expression : public Operation
{
    Just_Expression(Shared<const Phrase> syntax)
    :
        Operation(std::move(syntax), true)
    {}

    // These functions are called during evaluation.
    virtual Value eval(Frame&) const override = 0;
    virtual void exec(Frame&, Executor&) const override;
};

// `Just_Action` is an implementation class, inherited by Operation
// classes whose instances are always actions. It provides a default
// for `is_expr_` and the `eval` virtual function.
struct Just_Action : public Operation
{
    Just_Action(Shared<const Phrase> syntax)
    :
        Operation(std::move(syntax), false)
    {}

    // These functions are called during evaluation.
    //virtual Value eval(Frame&) const override;
    virtual void exec(Frame&, Executor&) const override = 0;
};

/// A Constant is an Expression whose value is known at compile time.
struct Constant : public Just_Expression
{
    Value value_;

    Constant(Shared<const Phrase> syntax, Value v)
    : Just_Expression(std::move(syntax)), value_(std::move(v))
    {
        // Constant expressions are pure. The tricky case is
        // Reactive_Expression values, which encapsulate an unevaluated
        // expression, which is required to be pure.
        pure_ = true;
    }

    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Null_Action : public Just_Action
{
    using Just_Action::Just_Action;
    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

// Nonlocal variable reference: in a recursively bound named function.
struct Symbolic_Ref : public Just_Expression
{
    Symbol_Ref name_;

    Symbolic_Ref(Shared<const Identifier> id)
    :
        Just_Expression(id),
        name_(id->symbol_)
    {}

    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
};

// Variable reference: module member, in init code for a module literal.
struct Module_Data_Ref : public Just_Expression
{
    slot_t slot_;
    slot_t index_;

    Module_Data_Ref(Shared<const Phrase> syntax, slot_t slot, slot_t index)
    : Just_Expression(std::move(syntax)), slot_(slot), index_(index)
    {}

    virtual Value eval(Frame&) const override;
};

// Nonlocal variable reference: in an anonymous or sequentially bound lambda.
struct Nonlocal_Data_Ref : public Just_Expression
{
    slot_t slot_;

    Nonlocal_Data_Ref(Shared<const Phrase> syntax, slot_t slot)
    : Just_Expression(std::move(syntax)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
};

// Metadata concerning a variable. The information is updated in phases.
//  0. The object is created when a variable is added to a Scope object,
//     and it is initialized to default values.
//  1. During analysis, if an assignment statement (:=) is used to mutate the
//     variable, then is_mutable is set to true.
//  2. During IR tree generation, when the variable definition is processed,
//     if the variable has an initialization expression (an IR_Expr), then it
//     is stored in ir_init_value_.
struct Scoped_Variable : public Shared_Base
{
    bool is_mutable_ = false;
    // Shared<const IR_Expr> ir_init_value_ = nullptr;
};

// Local variable reference: function parameter, let/where/local/for variable.
struct Local_Data_Ref : public Just_Expression
{
    slot_t slot_;
    Shared<const Scoped_Variable> variable_;

    Local_Data_Ref(Shared<const Phrase> syntax, slot_t slot,
        Shared<const Scoped_Variable> var)
    :
        Just_Expression(std::move(syntax)),
        slot_(slot),
        variable_(var)
    {
    }

    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
};

struct Call_Expr : public Just_Expression
{
    Shared<Operation> func_;
    Shared<Operation> arg_;

    Call_Expr(
        Shared<const Phrase> syntax,
        Shared<Operation> func,
        Shared<Operation> arg)
    :
        Just_Expression(std::move(syntax)),
        func_(std::move(func)),
        arg_(std::move(arg))
    {
        pure_ = (func_->pure_ && arg_->pure_);
    }

    virtual Value eval(Frame&) const override;
    virtual void tail_eval(std::unique_ptr<Frame>&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Prefix_Expr_Base : public Just_Expression
{
    Shared<Operation> arg_;

    Prefix_Expr_Base(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Expression(syntax),
        arg_(std::move(arg))
    {
        pure_ = arg_->pure_;
    }
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
};

struct Spread_Op : public Just_Action
{
    Shared<Operation> arg_;

    Spread_Op(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(syntax),
        arg_(std::move(arg))
    {}

    virtual void exec(Frame&, Executor&) const override;
};

struct Infix_Expr_Base : public Just_Expression
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;

    Infix_Expr_Base(
        Shared<const Phrase> syntax,
        Shared<Operation> arg1,
        Shared<Operation> arg2)
    :
        Just_Expression(syntax),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {
        pure_ = (arg1_->pure_ && arg2_->pure_);
    }
};
struct Predicate_Assertion_Expr : public Infix_Expr_Base
{
    Predicate_Assertion_Expr(
        Shared<const Phrase> syntax,
        Shared<Operation> arg1,
        Shared<Operation> arg2)
    :
        Infix_Expr_Base(std::move(syntax),std::move(arg1),std::move(arg2))
    {}
    virtual Value eval(Frame&) const override;
    //virtual SC_Value sc_eval(SC_Frame&) const override;
};
struct Or_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct And_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct Not_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct Index_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct Slice_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void print_repr(std::ostream& out, Prec) const override;
};
struct Range_Expr : public Just_Expression
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;
    Shared<Operation> arg3_;
    bool half_open_;

    Range_Expr(
        Shared<const Phrase> syntax,
        Shared<Operation> arg1,
        Shared<Operation> arg2,
        Shared<Operation> arg3,
        bool half_open)
    :
        Just_Expression(syntax),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3)),
        half_open_(half_open)
    {}
    virtual Value eval(Frame&) const override;
};

struct List_Expr_Base : public Just_Expression
{
    List_Expr_Base(Shared<const Phrase> syntax)
    : Just_Expression(std::move(syntax)) {}

    void init(); // call after construction & initialization of array elements
    virtual Value eval(Frame&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
    virtual void print_repr(std::ostream&, Prec) const override;
    TAIL_ARRAY_MEMBERS(Shared<Operation>)
};
struct List_Expr : public Tail_Array<List_Expr_Base>
{
    using Tail_Array<List_Expr_Base>::Tail_Array;
};

struct Paren_List_Expr_Base : public List_Expr_Base
{
    using List_Expr_Base::List_Expr_Base;
    virtual void exec(Frame&, Executor&) const override;
};
struct Paren_List_Expr : public Tail_Array<Paren_List_Expr_Base>
{
    using Tail_Array<Paren_List_Expr_Base>::Tail_Array;
};

// Used by the SubCurv compiler, which treats List_Expr and Paren_List_Expr
// identically. TODO: remove when (a,b,c) is no longer an expression.
inline Shared<const List_Expr> cast_list_expr(const Operation& op)
{
    if (auto le = dynamic_cast<const List_Expr*>(&op))
        return share(*le);
    if (auto ple = dynamic_cast<const Paren_List_Expr*>(&op))
        return share(*(const List_Expr*)ple);
    return nullptr;
}

struct Record_Expr : public Just_Expression
{
    Shared<const Operation> fields_;

    Record_Expr(Shared<const Phrase> syntax, Shared<const Operation> fields)
    :
        Just_Expression(syntax),
        fields_(fields)
    {}

    virtual Value eval(Frame&) const override;
};

/// The definitions and actions in a module or block compile into this.
struct Scope_Executable
{
    // For a module constructor, location in the evaluation frame where the
    // module is stored. For a block, (slot_t)(-1).
    slot_t module_slot_ = -1;

    // For a module constructor, the field dictionary.
    // For a block, nullptr.
    Shared<Module::Dictionary> module_dictionary_ = nullptr;

    // actions to execute at runtime: action statements and slot initialization
    std::vector<Shared<const Operation>> actions_ = {};

    Scope_Executable() {}

    /// Initialize the module slot, execute the definitions and action list.
    /// Return the module.
    Shared<Module> eval_module(Frame&) const;
    void exec(Frame&) const;
    void sc_exec(SC_Frame&) const;
};

struct Module_Expr : public Just_Expression
{
    using Just_Expression::Just_Expression;
    virtual Value eval(Frame&) const override;
    virtual Shared<Module> eval_module(Frame&) const = 0;
};

struct Const_Module_Expr final : public Module_Expr
{
    Shared<Module> value_;

    Const_Module_Expr(
        Shared<const Phrase> syntax,
        Shared<Module> value)
    :
        Module_Expr(syntax),
        value_(value)
    {}

    virtual Shared<Module> eval_module(Frame&) const override
    {
        return value_;
    }
};

struct Enum_Module_Expr final : public Module_Expr
{
    Shared<Module::Dictionary> dictionary_;
    std::vector<Shared<Operation>> exprs_;

    Enum_Module_Expr(
        Shared<const Phrase> syntax,
        Shared<Module::Dictionary> dictionary,
        std::vector<Shared<Operation>> exprs)
    :
        Module_Expr(syntax),
        dictionary_(dictionary),
        exprs_(exprs)
    {}

    virtual Shared<Module> eval_module(Frame&) const override;
};

struct Scoped_Module_Expr : public Module_Expr
{
    Scope_Executable executable_;

    Scoped_Module_Expr(
        Shared<const Phrase> syntax,
        Scope_Executable executable)
    :
        Module_Expr(syntax),
        executable_(std::move(executable))
    {}

    virtual Shared<Module> eval_module(Frame&) const override;
};

// An internal action for initializing the slots of a data definition
// in the evaluation frame. Part of the actions_ list in a Scope_Executable.
struct Data_Setter : public Just_Action
{
    slot_t module_slot_; // copied from enclosing Scope_Executable
    Shared<Pattern> pattern_;
    Shared<Operation> definiens_;

    Data_Setter(
        Shared<const Phrase> syntax,
        slot_t module_slot,
        Shared<Pattern> pattern,
        Shared<Operation> definiens)
    :
        Just_Action(std::move(syntax)),
        module_slot_(module_slot),
        pattern_(std::move(pattern)),
        definiens_(std::move(definiens))
    {}

    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

// An internal action for initializing the slots in the evaluation frame for
// a single non-recursive closure, or a group of mutually recursive closures.
// The closures share a single `nonlocals` object.
// Part of the actions_ list in a Scope_Executable for a Recursive_Scope.
struct Function_Setter_Base : public Just_Action
{
    // a copy of module_slot_ from the enclosing Scope_Executable.
    slot_t module_slot_;

    // construct the shared nonlocals object at runtime.
    Shared<Enum_Module_Expr> nonlocals_;

    Function_Setter_Base(
        Shared<const Phrase> syntax,
        slot_t module_slot,
        Shared<Enum_Module_Expr> nonlocals)
    :
        Just_Action(std::move(syntax)),
        module_slot_(module_slot),
        nonlocals_(std::move(nonlocals))
    {}

    virtual void exec(Frame&, Executor&) const override;

    struct Element {
        slot_t slot_;
        Shared<Lambda> lambda_;
        Element(slot_t s, Shared<Lambda> l);
        Element() noexcept;
    };
    TAIL_ARRAY_MEMBERS(Element)
};
struct Function_Setter : public Tail_Array<Function_Setter_Base>
{
    using Tail_Array<Function_Setter_Base>::Tail_Array;
};

struct Include_Setter_Base : public Just_Action
{
    slot_t module_slot_ = (slot_t)(-1);

    using Just_Action::Just_Action;

    virtual void exec(Frame&, Executor&) const override;

    struct Element {
        slot_t slot_;
        Value value_;
        Element(slot_t s, Value v) : slot_(s), value_(v) {}
        Element() noexcept {}
    };
    TAIL_ARRAY_MEMBERS(Element)
};
struct Include_Setter : public Tail_Array<Include_Setter_Base>
{
    using Tail_Array<Include_Setter_Base>::Tail_Array;
};

struct Compound_Op_Base : public Just_Action
{
    Compound_Op_Base(Shared<const Phrase> syntax)
    : Just_Action(std::move(syntax)) {}

    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;

    TAIL_ARRAY_MEMBERS(Shared<Operation>)
};
struct Compound_Op : public Tail_Array<Compound_Op_Base>
{
    using Tail_Array<Compound_Op_Base>::Tail_Array;
};

// Execute a statement list, then evaluate the body, which is an expression.
struct Do_Expr : public Just_Expression
{
    Shared<const Operation> actions_;
    Shared<const Operation> body_;

    Do_Expr(
        Shared<const Phrase> syntax,
        Shared<const Operation> a,
        Shared<const Operation> body)
    :
        Just_Expression(std::move(syntax)),
        actions_(std::move(a)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void tail_eval(std::unique_ptr<Frame>&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
};

struct Block_Op : public Operation
{
    Scope_Executable statements_;
    Shared<const Operation> body_;

    Block_Op(
        Shared<const Phrase> syntax,
        Scope_Executable b,
        Shared<const Operation> body)
    :
        Operation(std::move(syntax), body->is_expr_),
        statements_(std::move(b)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void tail_eval(std::unique_ptr<Frame>&) const override;
    virtual void exec(Frame&, Executor&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

struct For_Op : public Just_Action
{
    Shared<const Pattern> pattern_;
    Shared<const Operation> list_;
    Shared<const Operation> cond_;
    Shared<const Operation> body_;

    For_Op(
        Shared<const Phrase> syntax,
        Shared<const Pattern> pattern,
        Shared<const Operation> list,
        Shared<const Operation> cond,
        Shared<const Operation> body)
    :
        Just_Action(std::move(syntax)),
        pattern_(std::move(pattern)),
        list_(std::move(list)),
        cond_(std::move(cond)),
        body_(std::move(body))
    {}

    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

struct While_Op : public Just_Action
{
    Shared<const Operation> cond_;
    Shared<const Operation> body_;

    While_Op(
        Shared<const Phrase> syntax,
        Shared<const Operation> cond,
        Shared<const Operation> body)
    :
        Just_Action(std::move(syntax)),
        cond_(std::move(cond)),
        body_(std::move(body))
    {}

    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

struct If_Op : public Just_Action
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;

    If_Op(
        Shared<const Phrase> syntax,
        Shared<Operation> arg1,
        Shared<Operation> arg2)
    :
        Just_Action(syntax),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}

    virtual Value eval(Frame&) const override; // error message: missing else
    virtual void exec(Frame&, Executor&) const override;
    virtual void sc_exec(SC_Frame&) const override;
};

struct If_Else_Op : public Operation
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;
    Shared<Operation> arg3_;

    If_Else_Op(
        Shared<const Phrase> syntax,
        Shared<Operation> arg1,
        Shared<Operation> arg2,
        Shared<Operation> arg3)
    :
        Operation(syntax, arg2->is_expr_ && arg3->is_expr_),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3))
    {
        pure_ = (arg1_->pure_ && arg2_->pure_ && arg3_->pure_);
    }

    virtual Value eval(Frame&) const override;
    virtual void tail_eval(std::unique_ptr<Frame>&) const override;
    virtual void exec(Frame&, Executor&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
    virtual void sc_exec(SC_Frame&) const override;
    virtual size_t hash() const noexcept override;
    virtual bool hash_eq(const Operation&) const noexcept override;
    virtual void print_repr(std::ostream&, Prec) const override;
};

struct Lambda_Expr : public Just_Expression
{
    Shared<const Pattern> pattern_;
    Shared<Operation> body_;
    Shared<Module_Expr> nonlocals_;
    slot_t nslots_;
    Symbol_Ref name_{}; // may be set by Function_Definition::analyse
    int argpos_ = 0; // may be set by Function_Definition::analyse

    Lambda_Expr(
        Shared<const Phrase> syntax,
        Shared<const Pattern> pattern,
        Shared<Operation> body,
        Shared<Module_Expr> nonlocals,
        slot_t nslots)
    :
        Just_Expression(syntax),
        pattern_(std::move(pattern)),
        body_(std::move(body)),
        nonlocals_(std::move(nonlocals)),
        nslots_(nslots)
    {}

    virtual Value eval(Frame&) const override;
};

struct Segment : public Shared_Base
{
    Shared<const Segment_Phrase> syntax_;
    Segment(Shared<const Segment_Phrase> syntax) : syntax_(std::move(syntax)) {}
    virtual void generate(Frame&, String_Builder&) const = 0;
};
struct Literal_Segment : public Segment
{
    Shared<const String> data_;
    Literal_Segment(Shared<const Segment_Phrase> syntax, Shared<const String> data)
    : Segment(std::move(syntax)), data_(std::move(data)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Ident_Segment : public Segment
{
    Shared<Operation> expr_;
    Ident_Segment(Shared<const Segment_Phrase> syntax, Shared<Operation> expr)
    : Segment(std::move(syntax)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Paren_Segment : public Segment
{
    Shared<Operation> expr_;
    Paren_Segment(Shared<const Segment_Phrase> syntax, Shared<Operation> expr)
    : Segment(std::move(syntax)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Bracket_Segment : public Segment
{
    Shared<Operation> expr_;
    Bracket_Segment(Shared<const Segment_Phrase> syntax, Shared<Operation> expr)
    : Segment(std::move(syntax)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Brace_Segment : public Segment
{
    Shared<Operation> expr_;
    Brace_Segment(Shared<const Segment_Phrase> syntax, Shared<Operation> expr)
    : Segment(std::move(syntax)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct String_Expr_Base : public Just_Expression
{
    String_Expr_Base(Shared<const Phrase> syntax)
    : Just_Expression(std::move(syntax)) {}

    virtual Value eval(Frame&) const override;
    Symbol_Ref eval_symbol(Frame&) const;
    TAIL_ARRAY_MEMBERS(Shared<Segment>)
};
struct String_Expr : public Tail_Array<String_Expr_Base>
{
    using Tail_Array<String_Expr_Base>::Tail_Array;
};

struct Symbol_Expr
{
    Shared<const Identifier> id_ = nullptr;
    Shared<const Operation> expr_ = nullptr;

    Symbol_Expr(Shared<const Identifier> id) : id_(id) {}
    Symbol_Expr(Shared<String_Expr> str) : expr_(str) {}
    Symbol_Expr(Shared<const Operation> expr) : expr_(expr) {}
    Symbol_Expr(Shared<Operation> expr) : expr_(expr) {}

    Shared<const Phrase> syntax() {
        if (id_) return id_; else return expr_->syntax_;
    }
    Symbol_Ref eval(Frame&) const;
};

struct Dot_Expr : public Just_Expression
{
    Shared<Operation> base_;
    Symbol_Expr selector_;

    Dot_Expr(
        Shared<const Phrase> syntax,
        Shared<Operation> base,
        Symbol_Expr selector)
    :
        Just_Expression(std::move(syntax)),
        base_(std::move(base)),
        selector_(std::move(selector))
    {}

    virtual Value eval(Frame&) const override;
};

struct Assoc : public Just_Expression
{
    Symbol_Expr name_;
    Shared<const Operation> definiens_;

    Assoc(
        Shared<const Phrase> syntax,
        Symbol_Expr name,
        Shared<const Operation> definiens)
    :
        Just_Expression(std::move(syntax)),
        name_(std::move(name)),
        definiens_(std::move(definiens))
    {}

    virtual void exec(Frame&, Executor&) const override;
    virtual Value eval(Frame&) const override;
};

struct Parametric_Expr : public Just_Expression
{
    Shared<Lambda_Expr> ctor_;

    Parametric_Expr(
        Shared<const Phrase> syntax,
        Shared<Lambda_Expr> ctor)
    :
        Just_Expression(std::move(syntax)),
        ctor_(std::move(ctor))
    {}

    virtual Value eval(Frame&) const override;
};

// A Locative representing a boxed local variable.
// Closely related to Local_Data_Ref.
struct Local_Locative : public Locative
{
    Local_Locative(Shared<const Phrase> syntax, slot_t slot)
    :
        Locative(std::move(syntax)),
        slot_(slot)
    {}
    slot_t slot_;

    virtual Value fetch(Frame&) const override;
    virtual void store(Frame&, Value, const At_Syntax&) const override;
    virtual SC_Type sc_print(SC_Frame&) const override;
};

struct Indexed_Locative : public Locative
{
    Indexed_Locative(
        Shared<const Phrase> syntax,
        Unique<const Locative> b, Shared<const Operation> i)
    :
        Locative(std::move(syntax)),
        base_(std::move(b)), index_(std::move(i))
    {}
    Unique<const Locative> base_;
    Shared<const Operation> index_;

    virtual Value fetch(Frame&) const override;
    virtual void store(Frame&, Value, const At_Syntax&) const override;
    virtual SC_Type sc_print(SC_Frame&) const override;
};

struct List_Locative : public Locative
{
    List_Locative(
        Shared<const Phrase> syntax,
        std::vector<Unique<const Locative>> locs)
    :
        Locative(std::move(syntax)),
        locs_(std::move(locs))
    {}
    std::vector<Unique<const Locative>> locs_;

    virtual Value fetch(Frame&) const override;
    virtual void store(Frame&, Value, const At_Syntax&) const override;
};

// 'locative := expression'
struct Assignment_Action : public Just_Action
{
    Unique<const Locative> locative_;
    Shared<const Operation> expr_;

    Assignment_Action(
        Shared<const Phrase> syntax,
        Unique<const Locative> locative,
        Shared<const Operation> expr)
    :
        Just_Action(std::move(syntax)),
        locative_(std::move(locative)),
        expr_(std::move(expr))
    {}

    virtual void exec(Frame&, Executor&) const override;
    void sc_exec(SC_Frame&) const override;
};

// 'locative ! function' means 'locative := function locative'
struct Mutate_Action : public Just_Action
{
    struct XForm {
        // A call_phrase of the form `loc!f1!f2!...!fn`.
        Shared<const Phrase> call_phrase_;
        // The expression form of the 'fn' phrase from above.
        Shared<const Operation> func_expr_;
    };
    Unique<const Locative> locative_;
    std::vector<XForm> transformers_; // in the order f1, f2, ...
    Mutate_Action(
        Shared<const Phrase> syn,
        Unique<const Locative> loc,
        std::vector<XForm> tx)
    :
        Just_Action(std::move(syn)),
        locative_(std::move(loc)),
        transformers_(std::move(tx))
    {}
    void exec(Frame&, Executor&) const override;
};

struct TPath_Expr : public Just_Expression
{
    std::vector<Shared<const Operation>> indexes_;

    TPath_Expr(
        Shared<const Phrase> syntax,
        std::vector<Shared<const Operation>> indexes)
    :
        Just_Expression(std::move(syntax)),
        indexes_(std::move(indexes))
    {}

    virtual Value eval(Frame&) const override;
};
struct TSlice_Expr : public Just_Expression
{
    Shared<const Operation> indexes_; // evaluates to a List

    TSlice_Expr(
        Shared<const Phrase> syntax,
        Shared<const Operation> indexes)
    :
        Just_Expression(std::move(syntax)),
        indexes_(std::move(indexes))
    {}

    virtual Value eval(Frame&) const override;
};

} // namespace curv
#endif // header guard
