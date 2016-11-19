// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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
    size_t frame_nslots;
    size_t frame_maxslots;

    Environ(Environ* p)
    :
        parent_(p),
        eval_frame_(p == nullptr ? nullptr : p->eval_frame_),
        frame_nslots(0),
        frame_maxslots(0)
    {}
    Shared<Meaning> lookup(const Identifier& id);
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
struct Definition : public aux::Shared_Base
{
    Shared<const Identifier> name_;
    Shared<Phrase> definiens_;

    Definition(
        Shared<const Identifier> name,
        Shared<Phrase> definiens)
    :
        name_(std::move(name)),
        definiens_(std::move(definiens))
    {}
};

/// This is used to analyze a set of module definitions, and in future,
/// record definitions, let definitions, or function parameters.
struct Bindings
{
    size_t cur_position_;
    Shared<Module::Dictionary> dictionary_;
    std::vector<Shared<const Phrase>> slot_phrases_;

    // First, construct the Binding_Analyzer:
    Bindings()
    :
        cur_position_(0),
        dictionary_(make<Module::Dictionary>()),
        slot_phrases_()
    {}

    bool is_recursive_function(size_t);

    // Second, add some bindings:
    void add_definition(Shared<Definition> def, curv::Environ& env);
    //void add_parameter(Shared<Phrase> phrase);

    // Third, construct an Environ from the bindings dictionary.
    struct Environ : public curv::Environ
    {
    protected:
        Bindings& bindings_;
    public:
        Environ(
            curv::Environ* p,
            Bindings& b)
        :
            curv::Environ(p),
            bindings_(b)
        {}
        virtual Shared<Meaning> single_lookup(const Identifier&);
    };

    // Fourth, analyze the binding phrases, and construct a list of compile
    // time Values (constants, lambdas or thunks),
    // using the above Environ if they are mutually recursive:
    Shared<List> analyze_values(Environ& env);

    // Fifth, construct a Let_Expr, Module_Expr, Record_Expr
    // or function parameter list.
};

Shared<Operation> analyze_op(const Phrase& ph, Environ& env);

} // namespace
#endif // header guard
