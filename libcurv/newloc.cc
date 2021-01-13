#include <libcurv/newloc.h>
#include <libcurv/tree.h>
#include <libcurv/symbol.h>
#include <libcurv/exception.h>
#include <libcurv/sc_context.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/num.h>
namespace curv {

struct TPath_Expr : public Just_Expression
{
    std::vector<Shared<const Operation>> indexes_;

    TPath_Expr(
        Shared<const Phrase> syntax,
        std::vector<Shared<const Operation>> indexes)
    :
        Just_Expression(std::move(syntax)),
        indexes_(std::move(indexes))
    {}

    virtual Value eval(Frame& f) const override
    {
        std::vector<Value> ivals;
        for (auto i : indexes_)
            ivals.push_back(i->eval(f));
        return make_tpath(&ivals[0], &ivals[ivals.size()]);
    }
};

struct TSlice_Expr : public Just_Expression
{
    Shared<const Operation> indexes_; // evaluates to a List

    TSlice_Expr(
        Shared<const Phrase> syntax,
        Shared<const Operation> indexes)
    :
        Just_Expression(std::move(syntax)),
        indexes_(std::move(indexes))
    {}

    virtual Value eval(Frame& f) const override
    {
        Value ival = indexes_->eval(f);
        auto ilist = ival.to<List>(At_Phrase(*indexes_->syntax_, f));
        return make_tslice(ilist->begin(), ilist->end());
    }
};

void analyse_indexed_locative(
    Shared<const Phrase> ph, Locative_Indexes& il, Environ& env, Interp terp)
{
    ph = strip_parens(ph);
    if (auto id = cast<const Identifier>(ph))
        il.locative_ = env.lookup_lvar(*id, terp.edepth());
    else if (auto dot = cast<const Dot_Phrase>(ph)) {
        analyse_indexed_locative(dot->left_, il, env, terp);
        if (auto id = cast<const Identifier>(dot->right_)) {
            Shared<const Operation> index =
                make<Constant>(id, id->symbol_.to_value());
            il.indexes_.push_back(index);
        }
        else if (auto brackets = cast<const Bracket_Phrase>(dot->right_)) {
            auto index = analyse_op(*brackets, env);
            Shared<const Operation> slice = make<TSlice_Expr>(brackets, index);
            il.indexes_.push_back(slice);
        }
        else {
            throw Exception(At_Phrase(*dot->right_, env),
                "invalid expression after '.'");
        }
    }
    else if (auto at = cast<const Apply_Lens_Phrase>(ph)) {
        analyse_indexed_locative(at->arg_, il, env, terp);
        auto index = analyse_op(*at->function_, env);
        il.indexes_.push_back(index);
    }
    else throw Exception(At_Phrase(*ph, env), "not a locative");
}

Shared<Operation> analyse_assignment(
    Shared<const Assignment_Phrase> asn, Environ& env, Interp terp)
{
    Locative_Indexes il;
    analyse_indexed_locative(asn->left_, il, env, terp);
    auto right = analyse_op(*asn->right_, env);
    if (il.indexes_.empty()) {
        Shared<const Phrase> ph = asn;
        Shared<Operation> op =
            make<Assignment_Action>(ph, il.locative_, right);
        return op;
    }
    else {
        Shared<const Operation> index;
        if (il.indexes_.size() == 1)
            index = il.indexes_.front();
        else
            index = make<TPath_Expr>(asn->left_, il.indexes_);
        Shared<Operation> op =
            make<Assign_Indexed_Locative>(asn, il.locative_, index, right);
        return op;
    }
}

void Assign_Indexed_Locative::exec(Frame& f, Executor&) const
{
    auto index = index_->eval(f);
    auto elems = newval_->eval(f);
    auto curval = locative_->fetch(f);
    auto newval = tree_amend(curval, index, elems, At_Phrase(*syntax_,f));
    locative_->store(f, newval);
}

Value sc_get_index(SC_Frame& f, Shared<const Operation> index)
{
    if (auto k = cast<const Constant>(index))
        return k->value_;
    else if (auto slice = cast<const TSlice_Expr>(index)) {
        if (auto ilist = cast<const List_Expr>(slice->indexes_)) {
            if (ilist->size() == 1) {
                if (k = cast<const Constant>(ilist->at(0)))
                    return k->value_;
            }
        }
    }
    throw Exception(At_SC_Phrase(index->syntax_, f), "unsupported array index");
}
void sc_print_index(SC_Frame& f,
    SC_Type loctype, Shared<const Phrase> locsyntax,
    Shared<const Operation> index,
    SC_Type newvaltype, Shared<const Phrase> newvalsyntax)
{
    if (!loctype.is_vec()) {
        throw Exception(At_SC_Phrase(locsyntax, f), stringify(
            "Indexed assignment for a variable of type ",loctype,
            " is not supported"));
    }
    Value ival = sc_get_index(f, index);
    if (ival.is_num()) {
        auto num = ival.to_num_unsafe();
        if (num_is_int(num)) {
            int i = num_to_int(num, 0, loctype.count()-1,
                At_SC_Phrase(index->syntax_, f));
            if (newvaltype != loctype.elem_type()) {
                throw Exception(At_SC_Phrase(newvalsyntax, f),
                    "Left side of assignment has wrong type");
            }
            f.sc_.out() << "[" << i << "]";
            return;
        }
    }
    throw Exception(At_SC_Phrase(index->syntax_, f), "unsupported array index");
}
void Assign_Indexed_Locative::sc_exec(SC_Frame& f) const
{
    SC_Value newval = sc_eval_op(f, *newval_);
    f.sc_.out() << "  ";
    locative_->sc_print(f);
    sc_print_index(f,
        locative_->sc_type(f), locative_->syntax_,
        index_, newval.type, newval_->syntax_);
    f.sc_.out() << "="<<newval<<";\n";
}

}
