// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_DEFINITION_H
#define LIBCURV_DEFINITION_H

#include <libcurv/analyser.h>

namespace curv {

struct Pattern;
struct Recursive_Scope;

// Syntactically,
//    '<definition>' is a recursive definition
//    'local <definition>' is a sequential definition
// In both cases, a <definition> phrase is represented by a Definition object.
struct Definition : public Shared_Base
{
    Shared<const Phrase> syntax_;

    Definition(
        Shared<const Phrase> syntax)
    :
        syntax_(std::move(syntax))
    {}

    virtual void analyse_sequential(Environ&) = 0;
    virtual Shared<Operation> add_to_sequential_scope(Scope&) = 0;
    virtual void add_to_recursive_scope(Recursive_Scope&) = 0;
};

// A unitary definition is one that occupies a single node in the dependency
// graph that is constructed while analysing a recursive scope. It can define
// multiple names.
struct Unitary_Definition : public Definition
{
    Unitary_Definition(
        Shared<const Phrase> syntax)
    :
        Definition(syntax)
    {}

    virtual void analyse_recursive(Environ&) = 0;
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
    slot_t slot_; // initialized by add_to_recursive_scope()
    Shared<Lambda> lambda_; // initialized by analyse()
    Shared<Operation> lambda_expr_; // initialized by analyse_sequential()

    Function_Definition(
        Shared<const Phrase> syntax,
        Shared<const Identifier> name,
        Shared<Lambda_Phrase> lambda_phrase)
    :
        Unitary_Definition(syntax),
        name_(std::move(name)),
        lambda_phrase_(std::move(lambda_phrase))
    {}

    virtual void analyse_sequential(Environ&) override;
    virtual Shared<Operation> add_to_sequential_scope(Scope&) override;
    virtual void add_to_recursive_scope(Recursive_Scope&) override;
    virtual void analyse_recursive(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

// A data definition is `pattern = expr` where `expr` is not a lambda.
// Data definitions cannot be recursive, and they can use pattern matching
// to bind multiple names.
struct Data_Definition : public Unitary_Definition
{
    Shared<Pattern> pattern_;
    Shared<Phrase> definiens_phrase_;
    Shared<Operation> definiens_expr_; // initialized by analyse()

    Data_Definition(
        Shared<const Phrase> syntax,
        Shared<Pattern> pattern,
        Shared<Phrase> definiens)
    :
        Unitary_Definition(syntax),
        pattern_(pattern),
        definiens_phrase_(std::move(definiens))
    {}

    virtual void analyse_sequential(Environ&) override;
    virtual Shared<Operation> add_to_sequential_scope(Scope&) override;
    virtual void add_to_recursive_scope(Recursive_Scope&) override;
    virtual void analyse_recursive(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

struct Include_Definition : public Unitary_Definition
{
    Shared<Phrase> arg_;
    Shared<Include_Setter> setter_; // initialized by add_to_recursive_scope()

    Include_Definition(
        Shared<const Phrase> syntax,
        Shared<Phrase> arg)
    :
        Unitary_Definition(syntax),
        arg_(std::move(arg))
    {}

    virtual void analyse_sequential(Environ&) override;
    Shared<Include_Setter> add_to_scope(Scope& scope);
    virtual Shared<Operation> add_to_sequential_scope(Scope&) override;
    virtual void add_to_recursive_scope(Recursive_Scope&) override;
    virtual void analyse_recursive(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

struct Test_Definition : public Unitary_Definition
{
    Shared<Phrase> arg_;
    Shared<Operation> setter_; // initialized by analyse()

    Test_Definition(
        Shared<const Phrase> syntax,
        Shared<Phrase> arg)
    :
        Unitary_Definition(syntax),
        arg_(std::move(arg))
    {}

    virtual void analyse_sequential(Environ&) override;
    virtual Shared<Operation> add_to_sequential_scope(Scope&) override;
    virtual void add_to_recursive_scope(Recursive_Scope&) override;
    virtual void analyse_recursive(Environ&) override;
    virtual Shared<Operation> make_setter(slot_t module_slot) override;
};

// A compound definition is `statement1;statement2;...` where each statement
// is an action or definition, and there is at least one definition.
struct Compound_Definition_Base : public Definition
{
    Compound_Definition_Base(Shared<const Phrase> syntax)
    : Definition(std::move(syntax)) {}

    virtual void analyse_sequential(Environ&) override;
    virtual Shared<Operation> add_to_sequential_scope(Scope&) override;
    virtual void add_to_recursive_scope(Recursive_Scope&) override;

    TAIL_ARRAY_MEMBERS(Shared<Definition>)
    void resize(size_t n) { size_ = n; }
};
using Compound_Definition = Tail_Array<Compound_Definition_Base>;

// A Scope is set of user-defined variables defined over a lexical scope.
struct Scope : public Environ
{
    struct Binding {
        slot_t slot_index_;
        unsigned unit_index_;
        Shared<Scoped_Variable> variable_;

        Binding(slot_t slot, unsigned unit, Shared<Scoped_Variable> var)
        :
            slot_index_(slot),
            unit_index_(unit),
            variable_(var)
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

    // Environ protocol
    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual Shared<Locative> single_lvar_lookup(const Identifier&) override;

    // Scope protocol. Default implementation is for a sequential scope.
    virtual unsigned add_unit(Shared<Unitary_Definition>) { return 0; }
    virtual std::pair<slot_t,Shared<const Scoped_Variable>> add_binding(
        Symbol_Ref, const Phrase&, unsigned unit);
};

struct Recursive_Scope : public Scope
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
    bool target_is_module_;
    Scope_Executable executable_ = {};
    std::vector<Unit> units_ = {};
    int scc_count_ = 0;
    std::vector<Unit*> scc_stack_ = {};
    std::vector<Unit*> analysis_stack_ = {};

    Recursive_Scope(
        Environ& parent, bool target_is_module, Shared<const Phrase> syntax)
    :
        Scope(parent),
        syntax_(std::move(syntax)),
        target_is_module_(target_is_module)
    {
        if (target_is_module)
            executable_.module_slot_ = make_slot();
    }

    // Environ
    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual Shared<Locative> single_lvar_lookup(const Identifier&) override;

    // Scope
    virtual std::pair<slot_t,Shared<const Scoped_Variable>> add_binding(
        Symbol_Ref, const Phrase&, unsigned unit) override;

    // Recursive_Scope (add_to_recursive_scope protocol)
    void analyse(Definition&);
    virtual unsigned add_unit(Shared<Unitary_Definition>) override;

    void analyse();
private:
    void analyse_unit(Unit&, const Identifier*);
    Shared<Operation> make_function_setter(size_t nunits, Unit** units);
};

Shared<Module_Expr> analyse_module(Definition&, Environ&);

} // namespace
#endif // header guard
