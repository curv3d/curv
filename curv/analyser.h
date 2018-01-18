// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ANALYSER_H
#define CURV_ANALYSER_H

#include <curv/meaning.h>
#include <curv/builtin.h>

namespace curv {

struct Environ
{
    Environ* parent_;
    System& system_;
    /// eval_frame_ is nullptr, unless we are analysing a script due to an
    /// evaluation-time call to `file`. It's used as an Exception Context,
    /// to add a stack trace to compile time errors.
    Frame* eval_frame_;
    slot_t frame_nslots_;
    slot_t frame_maxslots_;

    /// true if this Environ represents a sequential statement list.
    bool is_sequential_statement_list_ = false;

    /// true if we are currently analysing an action statement within a
    /// sequential statement list: set and restored by analyse_action().
    bool is_analysing_action_ = false;

    // constructor for root environment
    Environ(System& system, Frame* eval_frame)
    :
        parent_(nullptr),
        system_(system),
        eval_frame_(eval_frame),
        frame_nslots_(0),
        frame_maxslots_(0)
    {}

    // constructor for child environment. parent is != nullptr.
    Environ(Environ* parent)
    :
        parent_(parent),
        system_(parent->system_),
        eval_frame_(parent->eval_frame_),
        frame_nslots_(0),
        frame_maxslots_(0)
    {}

    slot_t make_slot()
    {
        slot_t slot = frame_nslots_++;
        if (frame_maxslots_ < frame_nslots_)
            frame_maxslots_ = frame_nslots_;
        return slot;
    }

    Shared<Meaning> lookup(const Identifier& id);
    Shared<Meaning> lookup_var(const Identifier& id);
    virtual Shared<Meaning> single_lookup(const Identifier&) = 0;
};

struct Builtin_Environ : public Environ
{
protected:
    const Namespace& names;
public:
    Builtin_Environ(const Namespace& n, System& s, Frame* f)
    :
        Environ(s, f),
        names(n)
    {}
    virtual Shared<Meaning> single_lookup(const Identifier&);
};

Shared<Operation> analyse_op(const Phrase& ph, Environ& env);
Shared<Operation> analyse_action(const Phrase& ph, Environ& env);
Shared<Operation> analyse_tail(const Phrase& ph, Environ& env);

// Evaluate the phrase as a constant expression in the builtin environment.
Value std_eval(const Phrase& ph, Environ& env);

} // namespace
#endif // header guard
