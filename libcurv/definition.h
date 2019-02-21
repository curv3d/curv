// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_DEFINITION_H
#define LIBCURV_DEFINITION_H

#include <libcurv/analyser.h>

namespace curv {

struct Scope;
struct Block_Scope;
struct Pattern;

struct Definition : public Shared_Base
{
    Shared<const Phrase> syntax_;
    enum Kind { k_recursive, k_sequential } kind_;

    Definition(
        Shared<const Phrase> syntax,
        Kind k)
    :
        syntax_(std::move(syntax)),
        kind_(k)
    {}

    virtual void add_to_scope(Block_Scope&) = 0;
};

// A unitary definition is one that occupies a single node in the dependency
// graph that is constructed while analysing a recursive scope. It can define
// multiple names.
struct Unitary_Definition : public Definition
{
    Unitary_Definition(
        Shared<const Phrase> syntax,
        Kind k)
    :
        Definition(syntax, k)
    {}

    virtual void analyse(Environ&) = 0;
    virtual Shared<Operation> make_setter(slot_t module_slot) = 0;
};

// A function definition is `f=<lambda>` or `f x=<expr>`.
// Only `=` style function definitions belong to this class, so it's potentially
// a recursive definition.
// Only a single name is bound. There is no pattern matching in the definiendum.
struct Function_Definition : public Unitary_Definition
{
    Shared<const Identifier> name_;
    Shared<Lambda_Phrase> lambda_phrase_;
    slot_t slot_; // initialized by add_to_scope()
    Shared<Lambda> lambda_; // initialized by analyse()

    Function_Definition(
        Shared<const Phrase> syntax,
        Shared<const Identifier> name,
        Shared<Lambda_Phrase> lambda_phrase)
    :
        Unitary_Definition(syntax, k_recursive),
        name_(std::move(name)),
        lambda_phrase_(std::move(lambda_phrase))
    {}

    virtual void add_to_scope(Block_Scope&) override;
    virtual void analyse(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

// A data definition is `pattern = expression` or `var pattern := expression`
// or a `var f x:=expr` style function definition.
// Data definitions cannot be recursive, and they can use pattern matching
// to bind multiple names.
struct Data_Definition : public Unitary_Definition
{
    Shared<const Phrase> definiendum_;
    Shared<Phrase> definiens_phrase_;
    Shared<Pattern> pattern_; // initialized by add_to_scope()
    Shared<Operation> definiens_expr_; // initialized by analyse()

    Data_Definition(
        Shared<const Phrase> syntax,
        Kind k,
        Shared<const Phrase> definiendum,
        Shared<Phrase> definiens)
    :
        Unitary_Definition(syntax, k),
        definiendum_(std::move(definiendum)),
        definiens_phrase_(std::move(definiens))
    {}

    virtual void add_to_scope(Block_Scope&) override;
    virtual void analyse(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};
struct Include_Definition : public Unitary_Definition
{
    Shared<Phrase> arg_;
    Shared<Include_Setter> setter_; // initialized by add_to_scope()

    Include_Definition(
        Shared<const Phrase> syntax,
        Shared<Phrase> arg)
    :
        Unitary_Definition(syntax, k_recursive),
        arg_(std::move(arg))
    {}

    virtual void add_to_scope(Block_Scope&) override;
    virtual void analyse(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

// A compound definition is `statement1;statement2;...` where each statement
// is an action or definition, and there is at least one definition.
struct Compound_Definition_Base : public Definition
{
    struct Element
    {
        Shared<const Phrase> phrase_;
        Shared<Definition> definition_;
    };

    Compound_Definition_Base(Shared<const Phrase> syntax)
    : Definition(std::move(syntax), k_recursive) {}

    virtual void add_to_scope(Block_Scope&) override;

    TAIL_ARRAY_MEMBERS(Element)
};
using Compound_Definition = Tail_Array<Compound_Definition_Base>;

struct Scope : public Environ
{
    struct Binding {
        slot_t slot_index_;
        unsigned unit_index_;

        Binding(slot_t slot, unsigned unit)
        :
            slot_index_(slot),
            unit_index_(unit)
        {}
    };

    Symbol_Map<Binding> dictionary_ = {};

    Scope(Environ& parent)
    :
        Environ(&parent)
    {
        frame_nslots_ = parent.frame_nslots_;
        frame_maxslots_ = parent.frame_maxslots_;
    }

    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual slot_t add_binding(Symbol, const Phrase&, unsigned unit);
};

struct Block_Scope : public Scope
{
    bool target_is_module_;
    Scope_Executable executable_ = {};

    Block_Scope(Environ& parent, bool target_is_module)
    :
        Scope(parent),
        target_is_module_(target_is_module)
    {
        if (target_is_module)
            executable_.module_slot_ = make_slot();
    }

    virtual slot_t add_binding(Symbol, const Phrase&, unsigned unit) override;

    virtual void analyse(Definition&) = 0;
    virtual void add_action(Shared<const Phrase>) = 0;
    virtual unsigned begin_unit(Shared<Unitary_Definition>) = 0;
    virtual void end_unit(unsigned, Shared<Unitary_Definition>) = 0;
};

struct Sequential_Scope : public Block_Scope
{
    unsigned nunits_ = 0;

    Sequential_Scope(Environ& parent, bool target_is_module)
    :
        Block_Scope(parent, target_is_module)
    {
        is_sequential_statement_list_ = true;
    }

    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual void analyse(Definition&) override;
    virtual void add_action(Shared<const Phrase>) override;
    virtual unsigned begin_unit(Shared<Unitary_Definition>) override;
    virtual void end_unit(unsigned, Shared<Unitary_Definition>) override;
};

struct Recursive_Scope : public Block_Scope
{
    struct Unit {
        enum State {
            k_not_analysed, k_analysis_in_progress, k_analysed
        };
        Shared<Unitary_Definition> def_;
        State state_ = k_not_analysed;
        int scc_ord_ = -1; // -1 until SCC assigned
        int scc_lowlink_ = -1;
        Symbol_Map<Shared<Operation>> nonlocals_ = {};

        Unit(Shared<Unitary_Definition> def) : def_(def) {}

        bool is_function() {
            return cast<Function_Definition>(def_) != nullptr;
        }
    };
    struct Function_Environ : public Environ {
        Recursive_Scope& scope_;
        Unit& unit_;
        Function_Environ(Recursive_Scope& scope, Unit& unit)
        :
            Environ(scope.parent_),
            scope_(scope),
            unit_(unit)
        {
            frame_nslots_ = scope.frame_nslots_;
            frame_maxslots_ = scope.frame_maxslots_;
            assert(unit.is_function());
        }
        virtual Shared<Meaning> single_lookup(const Identifier&) override;
    };

    Shared<const Phrase> syntax_;
    std::vector<Shared<const Phrase>> action_phrases_ = {};
    std::vector<Unit> units_ = {};
    int scc_count_ = 0;
    std::vector<Unit*> scc_stack_ = {};
    std::vector<Unit*> analysis_stack_ = {};

    Recursive_Scope(
        Environ& parent, bool target_is_module, Shared<const Phrase> syntax)
    :
        Block_Scope(parent, target_is_module),
        syntax_(std::move(syntax))
    {
    }

    void analyse_unit(Unit&, const Identifier*);
    Shared<Operation> make_function_setter(size_t nunits, Unit** units);
    void analyse();

    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual void analyse(Definition&) override;
    virtual void add_action(Shared<const Phrase>) override;
    virtual unsigned begin_unit(Shared<Unitary_Definition>) override;
    virtual void end_unit(unsigned, Shared<Unitary_Definition>) override;
};

Shared<Module_Expr>
analyse_module(Definition&, Environ&);

} // namespace
#endif // header guard
