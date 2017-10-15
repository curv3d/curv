// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ANALYZER_H
#define CURV_ANALYZER_H

#include <curv/meaning.h>
#include <curv/builtin.h>

namespace curv {

struct Environ
{
    Environ* parent_;
    /// eval_frame_ is nullptr, unless we are analyzing a script due to an
    /// evaluation-time call to `file`. It's used as an Exception Context,
    /// to add a stack trace to compile time errors.
    Frame* eval_frame_;
    slot_t frame_nslots_;
    slot_t frame_maxslots_;

    /// true if this Environ represents a sequential statement list.
    bool is_sequential_statement_list_ = false;

    /// true if we are currently analyzing an action statement within a
    /// sequential statement list: set and restored by analyze_action().
    bool is_analyzing_action_ = false;

    Environ(Environ* p)
    :
        parent_(p),
        eval_frame_(p == nullptr ? nullptr : p->eval_frame_),
        frame_nslots_(0),
        frame_maxslots_(0)
    {}
    Shared<Meaning> lookup(const Identifier& id);
    Shared<Meaning> lookup_var(const Identifier& id);
    virtual Shared<Meaning> single_lookup(const Identifier&) = 0;
};

struct Builtin_Environ : public Environ
{
protected:
    const Namespace& names;
public:
    Builtin_Environ(const Namespace& n, Frame* f)
    :
        Environ(nullptr),
        names(n)
    {
        eval_frame_ = f;
    }
    virtual Shared<Meaning> single_lookup(const Identifier&);
};

/// A Definition is a partially analyzed phrase that binds a name to a value.
/// (In the future, a Definition can bind multiple names.)
///
/// There are multiple syntactic forms for definitions, and they are all
/// converted to Definition objects by Phrase::analyze_def(), which provides
/// a common interface for further analysis.
///
/// The definiens is not analyzed. In a recursive scope, we need to catalogue
/// all of the names bound within the scope before we analyze any of the
/// definientia.
struct Definition : public Shared_Base
{
    Shared<const Phrase> source_;
    Shared<const Identifier> name_;
    Shared<Phrase> definiens_;
    enum Kind { k_recursive, k_sequential } kind_;

    Definition(
        Shared<const Phrase> source,
        Shared<const Identifier> name,
        Shared<Phrase> definiens,
        Kind k)
    :
        source_(std::move(source)),
        name_(std::move(name)),
        definiens_(std::move(definiens)),
        kind_(k)
    {}
};

/// Analyze a sequence of definitions and actions,
/// as found in a block or a module literal.
struct Statement_Analyzer : public Environ
{
    struct Action_Phrase
    {
        int seq_no_;
        Shared<const Phrase> phrase_;

        Action_Phrase(int seq_no, Shared<const Phrase> phrase)
        : seq_no_(seq_no), phrase_(std::move(phrase))
        {}
    };
    struct Binding
    {
        slot_t slot_;
        int seq_no_;
        Shared<const Definition> def_;

        Binding(slot_t slot, int seq_no, Shared<const Definition> def)
        : slot_(slot), seq_no_(seq_no), def_(std::move(def))
        {}

        bool is_function_definition();
        bool defined_at_position(int pos);
        bool is_recursive();
    };
    Definition::Kind kind_;
    int seq_count_ = 0; // total# of seq pts (actions & seq. defs.)
    int cur_pos_; // set during analysis to seq# of stmt being analyzed
    slot_t slot_count_ = 0;
    Atom_Map<Binding> def_dictionary_ = {};
    Module::Dictionary nonlocal_dictionary_ = {};
    std::vector<Action_Phrase> action_phrases_ = {};
    Statements statements_ = {};
    bool target_is_module_;

    // First, construct the Statement_Analyzer:
    Statement_Analyzer(
        Environ& parent,
        Definition::Kind kind,
        bool target_is_module)
    :
        Environ(&parent),
        kind_(kind),
        target_is_module_(target_is_module)
    {
        frame_nslots_ = parent.frame_nslots_;
        frame_maxslots_ = parent.frame_maxslots_;
    }

    struct Thunk_Environ : public Environ
    {
        Statement_Analyzer& analyzer_;
        Thunk_Environ(Statement_Analyzer& analyzer)
        :
            Environ(analyzer.parent_),
            analyzer_(analyzer)
        {}
        virtual Shared<Meaning> single_lookup(const Identifier&);
    };

    virtual Shared<Meaning> single_lookup(const Identifier&);

    // Second, add some statements (definitions or actions):
    void add_statement(Shared<Phrase> statement);

    // Third, analyze the statements and finalize the data members.
    void analyze(Shared<const Phrase> source);

    // Finally, move or copy the desired data members to construct a Meaning.
    Shared<Module::Dictionary> make_module_dictionary();
};

Shared<Operation> analyze_op(const Phrase& ph, Environ& env);
Shared<Operation> analyze_action(const Phrase& ph, Environ& env);
Shared<Operation> analyze_tail(const Phrase& ph, Environ& env);

} // namespace
#endif // header guard
