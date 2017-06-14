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

namespace curv {

struct Operation;

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
/// There are 3 kinds of Operation:
///  1. An Expression is evaluated to produce a single value using `eval`.
///     Every expression is also a generator that produces 1 value.
///     For example, `2+2`.
///  2. A Generator is executed to produce a sequence of zero or more values
///     using `generate`. (Every Operation is also a generator.)
///     For example, `for(i=1..10)i^2`.
///  3. An Action is executed to cause a side effect using `exec`,
///     and no value is produced.
///     Every action is also a generator that produces 0 values.
///     For example, `assert(x>0)`.
struct Operation : public Meaning
{
    using Meaning::Meaning;

    // These functions are called during semantic analysis.
    virtual Shared<Operation> to_operation(Frame*);
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&);

    // These functions are called during evaluation.
    virtual Value eval(Frame&) const;
    virtual void generate(Frame&, List_Builder&) const = 0;
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
};

/// reference to a lazy nonlocal slot.
struct Nonlocal_Lazy_Ref : public Just_Expression
{
    slot_t slot_;

    Nonlocal_Lazy_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Indirect_Lazy_Ref : public Just_Expression
{
    slot_t slot_;
    slot_t index_;

    Indirect_Lazy_Ref(Shared<const Phrase> source, slot_t slot, slot_t index)
    : Just_Expression(std::move(source)), slot_(slot), index_(index)
    {}

    virtual Value eval(Frame&) const override;
};

struct Indirect_Strict_Ref : public Just_Expression
{
    slot_t slot_;
    slot_t index_;

    Indirect_Strict_Ref(Shared<const Phrase> source, slot_t slot, slot_t index)
    : Just_Expression(std::move(source)), slot_(slot), index_(index)
    {}

    virtual Value eval(Frame&) const override;
};

/// reference to a strict nonlocal slot (nonrecursive lambda nonlocal)
struct Nonlocal_Strict_Ref : public Just_Expression
{
    slot_t slot_;

    Nonlocal_Strict_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Let_Ref : public Just_Expression
{
    slot_t slot_;

    Let_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Arg_Ref : public Just_Expression
{
    slot_t slot_;

    Arg_Ref(Shared<const Phrase> source, slot_t slot)
    : Just_Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Nonlocal_Function_Ref : public Just_Expression
{
    slot_t lambda_slot_; ///! nonlocal slot containing Lambda value

    Nonlocal_Function_Ref(
        Shared<const Phrase> source,
        slot_t lambda_slot)
    :
        Just_Expression(std::move(source)),
        lambda_slot_(lambda_slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Indirect_Function_Ref : public Just_Expression
{
    slot_t slot_;
    slot_t index_;

    /// A prefix of the value list, of length `nlazy_`, may contain thunks
    /// which can only be evaluated in the caller's frame.
    slot_t nlazy_;

    Indirect_Function_Ref(
        Shared<const Phrase> source,
        slot_t slot, slot_t index, slot_t nlazy)
    :
        Just_Expression(std::move(source)),
        slot_(slot),
        index_(index),
        nlazy_(nlazy)
    {}

    virtual Value eval(Frame&) const override;
};

struct Dot_Expr : public Just_Expression
{
    Shared<Operation> base_;
    Atom id_;

    Dot_Expr(Shared<const Phrase> source, Shared<Operation> base, Atom id)
    :
        Just_Expression(std::move(source)),
        base_(std::move(base)),
        id_(std::move(id))
    {}

    virtual Value eval(Frame&) const override;
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

struct Spread_Gen : public Operation
{
    Shared<Operation> arg_;

    Spread_Gen(
        Shared<const Phrase> source,
        Shared<Operation> arg)
    :
        Operation(source),
        arg_(std::move(arg))
    {}

    virtual void generate(Frame&, List_Builder&) const override;
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
struct At_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};

struct Range_Gen : public Operation
{
    Shared<Operation> arg1_;
    Shared<Operation> arg2_;
    Shared<Operation> arg3_;
    bool half_open_;

    Range_Gen(
        Shared<const Phrase> source,
        Shared<Operation> arg1,
        Shared<Operation> arg2,
        Shared<Operation> arg3,
        bool half_open)
    :
        Operation(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3)),
        half_open_(half_open)
    {}
    virtual void generate(Frame&, List_Builder&) const override;
};

struct List_Expr_Base : public Just_Expression,
    public aux::Tail_Array_Data<Shared<Operation>>
{
    List_Expr_Base(Shared<const Phrase> source)
    : Just_Expression(std::move(source)) {}

    virtual Value eval(Frame&) const override;
    Shared<List> eval_list(Frame&) const;
    virtual GL_Value gl_eval(GL_Frame&) const override;
};
using List_Expr = aux::Tail_Array<List_Expr_Base>;

struct Record_Expr : public Just_Expression
{
    Atom_Map<Shared<const Operation>> fields_;

    Record_Expr(Shared<const Phrase> source) : Just_Expression(source) {}

    virtual Value eval(Frame&) const override;
};

/// 'Statements' represents the definitions and actions in a module literal
/// or block.
///
/// A mix of recursive and sequential definitions is supported, however, the
/// recursive definitions must come first, and the sequential definitions are
/// not visible within the recursive definitions. Removing this restriction
/// would entail complex, arbitrary semantics, a complex implementation, and
/// more complex documentation.
///
/// At runtime, a slot in the evaluation frame (slot_) contains the value list.
/// The value list is used as the nonlocal list for the closures constructed
/// from each top-level function definition. If this Statements is used to
/// construct a Module value, then the value list is stored in the Module.
/// TODO: As an optimization, we could store the value list directly in the
///       frame, if the definitions are sequential, and do not form a module,
///       and there are no function definitions.
///
/// The value list has a value for each binding, followed by "nonlocal values".
///  1. The value for a function definition is a Lambda. This is combined
///     with the value list to construct a Closure value when the function
///     is referenced (see Indirect_Function_Ref). We can't store the Closure
///     directly in the value list because that would create a reference cycle,
///     which would cause a storage leak, since we use reference counting.
///  2. The value for a non-function recursive definition is a Thunk.
///     The Thunk is evaluated and replaced by a Value on first reference,
///     see Indirect_Lazy_Ref. This allows definitions to be written in any order.
///     The order of definition evaluation is determined at runtime by data
///     dependencies, and can change from one evaluation to the next, affording
///     flexibility which is also available in Haskell. Unlike Haskell,
///     some recursive definitions will abort, reporting illegal recursion,
///     instead of looping forever. For example, `{x=x;}`.
///      * TODO: Once I support pattern matching definitions, like `(x,y)=f(a)`,
///        then the slots for x and y are initialized with the same action
///        thunk, which updates both slots.
///  3. The value for a non-function sequential definition is set to
///     a proper value by an action in the action list.
///  4. The value list may contain "nonlocal values", which correspond to
///     bindings from the parent scope which are referenced by function
///     definitions. These are proper values, not Thunks.
struct Statements
{
    // location in the evaluation frame where the value list is stored.
    slot_t slot_;

    // size and initial contents of the value list.
    Shared<const List> defn_values_;
    std::vector<Shared<const Operation>> nonlocal_exprs_;

    // actions to execute, during construction
    std::vector<Shared<const Operation>> actions_;

    Statements(
        slot_t slot,
        Shared<const List> defn_values,
        std::vector<Shared<const Operation>> nonlocal_exprs,
        std::vector<Shared<const Operation>> actions)
    :
        slot_(slot),
        defn_values_(std::move(defn_values)),
        nonlocal_exprs_(std::move(nonlocal_exprs)),
        actions_(std::move(actions))
    {}

    Statements() {}

    /// Initialize the Frame slot, execute the definitions and action list.
    /// Return the value list.
    Shared<List> eval(Frame&) const;
    void exec(Frame&) const;
    void gl_exec(GL_Frame&) const;
};

// An internal action for storing the value of a sequential definition
// in the evaluation frame. Part of the actions_ list in a Statements.
struct Let_Assign : public Just_Action
{
    slot_t slot_;
    Shared<Operation> expr_;
    bool reassign_;

    Let_Assign(
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

// An internal action for storing the value of a sequential definition
// in the evaluation frame. Part of the actions_ list in a Statements.
struct Indirect_Assign : public Just_Action
{
    slot_t slot_;
    slot_t index_;
    Shared<Operation> expr_;

    Indirect_Assign(
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

// A module expression is `{stmt; stmt; ...;}` where stmt is a definition
// or action. The scope of each definition is the entire module. The order
// of definitions doesn't matter. Recursive definitions are supported.
// Actions are executed in left-to-right order.
struct Module_Expr : public Just_Expression
{
    // maps public member names to slot #s in the value list.
    Shared<Module::Dictionary> dictionary_;

    Statements statements_;

    Module_Expr(
        Shared<const Phrase> source,
        Shared<Module::Dictionary> dictionary,
        Statements statements)
    :
        Just_Expression(source),
        dictionary_(std::move(dictionary)),
        statements_(std::move(statements))
    {}

    virtual Value eval(Frame&) const override;
    Shared<Module> eval_module(Frame&) const;
};

struct Block_Op : public Operation
{
    Statements statements_;
    Shared<const Operation> body_;

    Block_Op(
        Shared<const Phrase> source,
        Statements b,
        Shared<const Operation> body)
    :
        Operation(std::move(source)),
        statements_(std::move(b)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
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
    virtual void exec(Frame&) const override;
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
    virtual void exec(Frame&) const override;
    virtual GL_Value gl_eval(GL_Frame&) const override;
    virtual void gl_exec(GL_Frame&) const override;
};

struct Lambda_Expr : public Just_Expression
{
    Shared<Operation> body_;
    Shared<List_Expr> nonlocals_;
    slot_t nargs_;
    slot_t nslots_;

    Lambda_Expr(
        Shared<const Phrase> source,
        Shared<Operation> body,
        Shared<List_Expr> nonlocals,
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

struct Assoc : public Meaning
{
    Shared<const Identifier> name_;
    Shared<const Operation> definiens_;

    Assoc(
        Shared<const Phrase> source,
        Shared<const Identifier> name,
        Shared<const Operation> definiens)
    :
        Meaning(std::move(source)),
        name_(std::move(name)),
        definiens_(std::move(definiens))
    {}
};

} // namespace curv
#endif // header guard
