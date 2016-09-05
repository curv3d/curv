// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ANALYZER_H
#define CURV_ANALYZER_H

#include <curv/meaning.h>
#include <curv/builtin.h>

namespace curv {

/// This is used to analyze a list of module definitions, record definitions,
/// let definitions, or function parameters.
struct Bindings_Analyzer
{
    size_t cur_position_;
    Shared<Module::Dictionary> dictionary_;
    std::vector<Shared<const Phrase>> slot_phrases_;

    // First, construct the Binding_Analyzer:
    Bindings_Analyzer()
    :
        cur_position_(0),
        dictionary_(aux::make_shared<Module::Dictionary>()),
        slot_phrases_()
    {}

    // Second, add some bindings:
    void add_definition(Shared<Phrase> phrase);
    //void add_parameter(Shared<Phrase> phrase);

    // Third, construct an Environ from the bindings dictionary.
    // I have no way to make this generic right now.

    // Fourth, analyze the binding phrases, and construct a list of compile
    // time Values (either constants or thunks),
    // using the above Environ if they are mutually recursive:
    Shared<List> analyze_values(Environ& env);

    // Fifth, construct a Let_Expr, Module_Expr, Record_Expr
    // or function parameter list.
};

struct Environ
{
    Environ* parent;
    size_t frame_nslots;
    size_t frame_maxslots;

    Environ(Environ* p = nullptr)
    : parent(p), frame_nslots(0), frame_maxslots(0)
    { }
    Shared<Meaning> lookup(const Identifier& id) const;
    virtual Shared<Meaning> single_lookup(const Identifier&, Atom) const = 0;
};

struct Builtin_Environ : public Environ
{
protected:
    const Namespace& names;
public:
    Builtin_Environ(const Namespace& n, Environ* p = nullptr)
    : Environ(p), names(n)
    {}
    virtual Shared<Meaning> single_lookup(const Identifier&, Atom) const;
};

struct Module_Environ : public Environ
{
protected:
    Shared<Module::Dictionary> dictionary_;
public:
    Module_Environ(
        Environ* p,
        Shared<Module::Dictionary> d)
    :
        Environ(p),
        dictionary_(std::move(d))
    {}
    virtual Shared<Meaning> single_lookup(const Identifier&, Atom) const;
};

inline Shared<Meaning>
analyze(const Phrase& ph, Environ& env)
{
    return ph.analyze(env);
}

Shared<Expression> analyze_expr(const Phrase& ph, Environ& env);

} // namespace
#endif // header guard
