// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_MEANING_H
#define CURV_MEANING_H

#include <curv/gl_compiler.h>
#include <vector>
#include <aux/tail_array.h>
#include <curv/shared.h>
#include <curv/phrase.h>
#include <curv/value.h>
#include <curv/atom.h>
#include <curv/list.h>
#include <curv/module.h>
#include <curv/frame.h>
#include <curv/record.h>

namespace curv {

struct Operation;
struct Lambda;

/// An abstract base class representing a semantically analyzed Phrase.
struct Meaning : public Shared_Base
{
    /// The original source code for this meaning.
    ///
    /// The syntax of the source code need not have any relation to the meaning
    /// class. Eg, an Identifier phrase can be analyzed into a variety of
    /// different meanings. That's why we separate the Phrase tree from the
    /// Meaning tree.
    Shared<const Phrase> source_;

    Meaning(Shared<const Phrase> source) : source_(std::move(source)) {}

    // These functions are called during semantic analysis.
    virtual Shared<Operation> to_operation(Frame*);
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&);
};

/// A Metafunction is a function that is called during analysis, instead of
/// at run time.
///
/// A call to a Metafunction is compiled to a Meaning using `call`.
/// Metafunctions enable the client to add new syntax to the language
/// without modifying the grammar or modifying the Curv library,
/// as long as that new syntax parses as a function call.
///
/// Metafunctions are not values, & Metafunction is not a subclass of Operation.
/// Metafunctions are similar to the macros of the Scheme and Rust languages,
/// but we currently have no plan to support user defined metafunctions.
struct Metafunction : public Meaning
{
    using Meaning::Meaning;
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&) override = 0;
};

/// An Operation is a fragment of compiled code that "does something" at run
/// time. During analysis, a Curv script is compiled into an Operation tree.
///
/// At present, the Operation tree has two roles. It is our "IR" (Intermediate
/// Representation) to which optimizations are applied, and it is also our
/// executable format. In the future, we should separate these roles, add a
/// separate code generation phase, and use a more efficient executable code
/// representaton.
///
/// There are 4 kinds of Operation:
///  1. An Expression is evaluated to produce a single value using `eval`.
///     Every expression is also a generator that produces 1 value.
///     For example, `2+2`.
///  2. A Generator is executed to produce a sequence of zero or more values
///     using `generate`. (Expressions and Actions are also generators.)
///     For example, `for(i=1..10)i^2`.
///  3. A Binder is executed to bind zero or more names to values
///     in a record value that is under construction, using ``bind``.
///     For example, `x : 42`.
///  4. An Action is executed to cause a side effect using `exec`,
///     and no value is produced.
///     Every action is also a generator that produces 0 values.
///     Every action is also a binder that binds no names.
///     For example, `assert(x>0)`.
struct Operation : public Meaning
{
    using Meaning::Meaning;

    // These functions are called during semantic analysis.
    virtual Shared<Operation> to_operation(Frame*);
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&);

    // These functions are called during evaluation.
    virtual Value eval(Frame&) const;
    virtual void generate(Frame&, List_Builder&) const;
    virtual void bind(Frame&, Record&) const;
    virtual void exec(Frame&) const;

    // These functions are called by the Geometry Compiler.
    virtual GL_Value gl_eval(GL_Frame&) const;
    virtual void gl_exec(GL_Frame&) const;
};

/// `Just_Expression` is an implementation class, inherited by Operation classes
/// whose instances are always expressions. It provides sensible defaults
/// for the eval/generate/exec virtual functions.
///
/// An expression is an Operation that can be evaluated to produce a single 
/// value. The work is done by the `eval` method, which must be defined.
/// All expressions are also generators that produce a single value,
/// so the `generate` function calls `eval`.
///
/// This is not an interface class, and not all expression objects are derived
/// from Just_Expression. Functions should not take Just_Expressions as values
/// or return Just_Expressions as results: use Operation instead.
struct Just_Expression : public Operation
{
    using Operation::Operation;

    // These functions are called during evaluation.
    virtual Value eval(Frame&) const override = 0;
    virtual void generate(Frame&, List_Builder&) const override;
};

/// `Just_Action` is an implementation class, inherited by Operation classes
/// whose instances are always actions. It provides sensible defaults
/// for the eval/generate/exec virtual functions.
///
/// An action is an Operation that causes a side effect and produces no values.
/// The work is done by the `exec` method, which must be defined.
/// All actions are generators that produce no values, so the `generate` method
/// calls the `exec` method.
///
/// This is not an interface class, and not all action objects are derived
/// from Just_Action. Functions should not take Just_Actions as values
/// or return Just_Actions as results: use Operation instead.
struct Just_Action : public Operation
{
    using Operation::Operation;

    // These functions are called during evaluation.
    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override = 0;
};

/// A Constant is an Expression whose value is known at compile time.
struct Constant : public Just_Expression
{
    Value value_;

    Constant(Shared<const Phrase> source, Value v)
    : Just_Expression(std::move(source)), value_(std::move(v))
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Null_Action : public Just_Action
{
    using Just_Action::Just_Action;
    virtual void exec(Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct Symbolic_Ref : public Just_Expression
{
    Atom name_;

    Symbolic_Ref(Shared<const Identifier> id)
    :
        Just_Expression(id),
        name_(id->atom_)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Module_Data_Ref : public Just_Expression
{
    slot_t slot_;
    slot_t index_;

    Module_Data_Ref(Shared<const Phrase> source, slot_t slot, slot_t index)
    : Just_Expression(std::move(source)), slot_(slot), index_(index)
    {}

    virtual Value eval(Frame&) const override;
};

/// reference to a strict nonlocal slot (nonrecursive lambda nonlocal)
struct Nonlocal_Data_Ref : public Just_Expression
{
    slot_t slot_;

    Nonlocal_Data_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Data_Ref : public Just_Expression
{
    slot_t slot_;

    Data_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Call_Expr : public Just_Expression
{
    Shared<Operation> fun_;
    Shared<Operation> arg_;

    Call_Expr(
        Shared<const Call_Phrase> source,
        Shared<Operation> fun,
        Shared<Operation> arg)
    :
        Just_Expression(std::move(source)),
        fun_(std::move(fun)),
        arg_(std::move(arg))
    {}

    inline const Call_Phrase* call_phrase() const
    {
        // This is safe because, by construction, the source_ field
        // is initialized from a Call_Phrase. See constructor, above.
        return (Call_Phrase*) &*source_;
    }

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Prefix_Expr_Base : public Just_Expression
{
    Shared<Operation> arg_;

    Prefix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Operation> arg)
    :
        Just_Expression(source),
        arg_(std::move(arg))
    {}
};
struct Not_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Positive_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Negative_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Spread_Op : public Operation
{
    Shared<Operation> arg_;

    Spread_Op(
        Shared<const Phrase> source,
        Shared<Operation> arg)
    :
        Operation(source),
        arg_(std::move(arg))
    {}

    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
};

struct Infix_Expr_Base : public Just_Expression
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;

    Infix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Operation> arg1,
        Shared<Operation> arg2)
    :
        Just_Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}
};
struct Or_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct And_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Not_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Less_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Greater_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Less_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Greater_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Add_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Subtract_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Multiply_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Divide_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Power_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
struct Index_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Range_Expr : public Just_Expression
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;
    Shared<Operation> arg3_;
    bool half_open_;

    Range_Expr(
        Shared<const Phrase> source,
        Shared<Operation> arg1,
        Shared<Operation> arg2,
        Shared<Operation> arg3,
        bool half_open)
    :
        Just_Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3)),
        half_open_(half_open)
    {}
    virtual Value eval(Frame&) const override;
};

struct List_Expr_Base : public Just_Expression
{
    List_Expr_Base(Shared<const Phrase> source)
    : Just_Expression(std::move(source)) {}

    virtual Value eval(Frame&) const override;
    Shared<List> eval_list(Frame&) const;
    virtual GL_Value gl_eval(GL_Frame&) const override;
    TAIL_ARRAY_MEMBERS(Shared<Operation>)
};
using List_Expr = aux::Tail_Array<List_Expr_Base>;

struct Record_Expr : public Just_Expression
{
    // `fields_` contains actions and binders.
    std::vector<Shared<const Operation>> fields_;

    Record_Expr(Shared<const Phrase> source) : Just_Expression(source) {}

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
    void gl_exec(GL_Frame&) const;
};

// An internal action for storing the value of a data definition
// in the evaluation frame. Part of the actions_ list in a Scope_Executable.
struct Data_Setter : public Just_Action
{
    slot_t slot_;
    Shared<Operation> expr_;
    bool reassign_;

    Data_Setter(
        Shared<const Phrase> source,
        slot_t slot,
        Shared<Operation> expr,
        bool reassign)
    :
        Just_Action(std::move(source)),
        slot_(slot),
        expr_(std::move(expr)),
        reassign_(reassign)
    {}

    void exec(Frame&) const override;
    void gl_exec(GL_Frame&) const override;
};

// An internal action for storing the value of a data definition
// in the evaluation frame. Part of the actions_ list in a Scope_Executable.
struct Module_Data_Setter : public Just_Action
{
    slot_t slot_;
    slot_t index_;
    Shared<Operation> expr_;

    Module_Data_Setter(
        Shared<const Phrase> source,
        slot_t slot,
        slot_t index,
        Shared<Operation> expr)
    :
        Just_Action(std::move(source)),
        slot_(slot),
        index_(index),
        expr_(std::move(expr))
    {}

    void exec(Frame&) const override;
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
        Shared<const Phrase> source,
        Shared<Module> value)
    :
        Module_Expr(source),
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
        Shared<const Phrase> source,
        Shared<Module::Dictionary> dictionary,
        std::vector<Shared<Operation>> exprs)
    :
        Module_Expr(source),
        dictionary_(dictionary),
        exprs_(exprs)
    {}

    virtual Shared<Module> eval_module(Frame&) const override;
};

struct Scoped_Module_Expr : public Module_Expr
{
    Scope_Executable executable_;

    Scoped_Module_Expr(
        Shared<const Phrase> source,
        Scope_Executable executable)
    :
        Module_Expr(source),
        executable_(std::move(executable))
    {}

    virtual Shared<Module> eval_module(Frame&) const override;
};

// An internal action for initializing the slots in the evaluation frame
// for a group of mutually recursive closures.
// The closures share a single `nonlocals` object.
// Part of the actions_ list in a Scope_Executable for a Recursive_Scope.
struct Function_Setter_Base : public Just_Action
{
    // a copy of module_slot_ from the enclosing Scope_Executable.
    slot_t module_slot_;

    // construct the shared nonlocals object at runtime.
    Shared<Enum_Module_Expr> nonlocals_;

    Function_Setter_Base(
        Shared<const Phrase> source,
        slot_t module_slot,
        Shared<Enum_Module_Expr> nonlocals)
    :
        Just_Action(std::move(source)),
        module_slot_(module_slot),
        nonlocals_(std::move(nonlocals))
    {}

    void exec(Frame&) const override;

    struct Element {
        slot_t slot_;
        Shared<Lambda> lambda_;
        Element(slot_t s, Shared<Lambda> l) : slot_(s), lambda_(l) {}
        Element() noexcept {}
    };
    TAIL_ARRAY_MEMBERS(Element)
};
using Function_Setter = aux::Tail_Array<Function_Setter_Base>;

struct Use_Setter_Base : public Just_Action
{
    slot_t module_slot_ = (slot_t)(-1);

    using Just_Action::Just_Action;

    void exec(Frame&) const override;

    using Element = std::pair<slot_t, Value>;
    TAIL_ARRAY_MEMBERS(Element)
};
using Use_Setter = aux::Tail_Array<Use_Setter_Base>;

struct Compound_Op_Base : public Operation
{
    Compound_Op_Base(Shared<const Phrase> source)
    : Operation(std::move(source)) {}

    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;

    TAIL_ARRAY_MEMBERS(Shared<Operation>)
};
using Compound_Op = aux::Tail_Array<Compound_Op_Base>;

// Execute some actions, then execute the body.
// This is a restricted form of block with no definitions.
struct Preaction_Op : public Operation
{
    Shared<const Operation> actions_;
    Shared<const Operation> body_;

    Preaction_Op(
        Shared<const Phrase> source,
        Shared<const Operation> a,
        Shared<const Operation> body)
    :
        Operation(std::move(source)),
        actions_(std::move(a)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct Block_Op : public Operation
{
    Scope_Executable statements_;
    Shared<const Operation> body_;

    Block_Op(
        Shared<const Phrase> source,
        Scope_Executable b,
        Shared<const Operation> body)
    :
        Operation(std::move(source)),
        statements_(std::move(b)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct For_Op : public Operation
{
    slot_t slot_;
    Shared<const Operation> list_;
    Shared<const Operation> body_;

    For_Op(
        Shared<const Phrase> source,
        slot_t slot,
        Shared<const Operation> list,
        Shared<const Operation> body)
    :
        Operation(std::move(source)),
        slot_(slot),
        list_(std::move(list)),
        body_(std::move(body))
    {}

    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct While_Action : public Just_Action
{
    Shared<const Operation> cond_;
    Shared<const Operation> body_;

    While_Action(
        Shared<const Phrase> source,
        Shared<const Operation> cond,
        Shared<const Operation> body)
    :
        Just_Action(std::move(source)),
        cond_(std::move(cond)),
        body_(std::move(body))
    {}

    virtual void exec(Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct If_Op : public Operation
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;

    If_Op(
        Shared<const Phrase> source,
        Shared<Operation> arg1,
        Shared<Operation> arg2)
    :
        Operation(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}

    virtual Value eval(Frame&) const override; // error message: missing else
    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct If_Else_Op : public Operation
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;
    Shared<Operation> arg3_;

    If_Else_Op(
        Shared<const Phrase> source,
        Shared<Operation> arg1,
        Shared<Operation> arg2,
        Shared<Operation> arg3)
    :
        Operation(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
    virtual void bind(Frame&, Record&) const override;
    virtual void exec(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct Lambda_Expr : public Just_Expression
{
    Shared<Operation> body_;
    Shared<Module_Expr> nonlocals_;
    slot_t nargs_;
    slot_t nslots_;

    Lambda_Expr(
        Shared<const Phrase> source,
        Shared<Operation> body,
        Shared<Module_Expr> nonlocals,
        slot_t nargs,
        slot_t nslots)
    :
        Just_Expression(source),
        body_(std::move(body)),
        nonlocals_(std::move(nonlocals)),
        nargs_(nargs),
        nslots_(nslots)
    {}

    virtual Value eval(Frame&) const override;
};

struct Segment : public Shared_Base
{
    Shared<const Segment_Phrase> source_;
    Segment(Shared<const Segment_Phrase> source) : source_(std::move(source)) {}
    virtual void generate(Frame&, String_Builder&) const = 0;
};
struct Literal_Segment : public Segment
{
    Shared<const String> data_;
    Literal_Segment(Shared<const Segment_Phrase> source, Shared<const String> data)
    : Segment(std::move(source)), data_(std::move(data)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Paren_Segment : public Segment
{
    Shared<Operation> expr_;
    Paren_Segment(Shared<const Segment_Phrase> source, Shared<Operation> expr)
    : Segment(std::move(source)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct Brace_Segment : public Segment
{
    Shared<Operation> expr_;
    Brace_Segment(Shared<const Segment_Phrase> source, Shared<Operation> expr)
    : Segment(std::move(source)), expr_(std::move(expr)) {}
    virtual void generate(Frame&, String_Builder&) const;
};
struct String_Expr_Base : public Just_Expression
{
    String_Expr_Base(Shared<const Phrase> source)
    : Just_Expression(std::move(source)) {}

    virtual Value eval(Frame&) const override;
    Atom eval_atom(Frame&) const;
    TAIL_ARRAY_MEMBERS(Shared<Segment>)
};
using String_Expr = aux::Tail_Array<String_Expr_Base>;

struct Atom_Expr
{
    Shared<const Identifier> id_;
    Shared<String_Expr> string_;

    Atom_Expr(Shared<const Identifier> id) : id_(id), string_(nullptr) {}
    Atom_Expr(Shared<String_Expr> string) : id_(nullptr), string_(string) {}

    Shared<const Phrase> source() {
        if (id_) return id_; else return string_->source_;
    }
    Atom eval(Frame& f) const {
        return id_ ? id_->atom_ : string_->eval_atom(f);
    } 
};

struct Dot_Expr : public Just_Expression
{
    Shared<Operation> base_;
    Atom_Expr selector_;

    Dot_Expr(
        Shared<const Phrase> source,
        Shared<Operation> base,
        Atom_Expr selector)
    :
        Just_Expression(std::move(source)),
        base_(std::move(base)),
        selector_(std::move(selector))
    {}

    virtual Value eval(Frame&) const override;
};

struct Assoc : public Operation
{
    Atom_Expr name_;
    Shared<const Operation> definiens_;

    Assoc(
        Shared<const Phrase> source,
        Atom_Expr name,
        Shared<const Operation> definiens)
    :
        Operation(std::move(source)),
        name_(std::move(name)),
        definiens_(std::move(definiens))
    {}

    virtual void bind(Frame&, Record&) const override;
};

} // namespace curv
#endif // header guard
