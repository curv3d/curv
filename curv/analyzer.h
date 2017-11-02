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
    Builtin_Environ(const Namespace& n, Frame* f)
    :
        Environ(nullptr),
        names(n)
    {
        eval_frame_ = f;
    }
    virtual Shared<Meaning> single_lookup(const Identifier&);
};

Shared<Operation> analyze_op(const Phrase& ph, Environ& env);
Shared<Operation> analyze_action(const Phrase& ph, Environ& env);
Shared<Operation> analyze_tail(const Phrase& ph, Environ& env);

} // namespace
#endif // header guard
