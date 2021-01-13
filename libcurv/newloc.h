#include <libcurv/meaning.h>
#include <libcurv/analyser.h>
#include <vector>
namespace curv {

struct Locative_Indexes
{
    Shared<Locative> locative_;
    std::vector<Shared<const Operation>> indexes_;
};

void analyse_indexed_locative(
    Shared<const Phrase>, Locative_Indexes&, Environ&, Interp);

Shared<Operation> analyse_assignment(
    Shared<const Assignment_Phrase> asn, Environ& env, Interp terp);

struct Assign_Indexed_Locative : public Operation
{
    Shared<const Locative> locative_;
    Shared<const Operation> index_;
    Shared<const Operation> newval_;

    Assign_Indexed_Locative(
        Shared<const Phrase> syntax,
        Shared<const Locative> locative,
        Shared<const Operation> index,
        Shared<const Operation> newval)
    :
        Operation(std::move(syntax)),
        locative_(std::move(locative)),
        index_(std::move(index)),
        newval_(std::move(newval))
    {}

    virtual void exec(Frame&, Executor&) const override;
    void sc_exec(SC_Frame&) const override;
};

/*
    On success, you get a Locative and a path (std::list of index Expressions).
    It recurses down to an Identifier.
Assign_Locative (has a Locative destination and a new_value Expression)
    loc.store(f, nv->eval(f), *nv->syntax_)
Assign_Indexed_Locative (has a Locative, index and elems expressions)
  ::exec(Frame& f, Executor&)
    index = index_expr->eval(f)
    elems = elems_expr->eval(f)
    curval = loc->fetch(f)
    newval = amend(curval, index, elems, At_Phrase(*syntax_,f))
    loc->store(f, newval, *elems_expr->syntax_);
*/

}
