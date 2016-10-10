// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/shared.h>
#include <curv/exception.h>
#include <curv/thunk.h>
#include <curv/function.h>

namespace curv
{

Shared<Expression>
analyze_expr(const Phrase& ph, Environ& env)
{
    Shared<Meaning> m = analyze(ph, env);
    Expression* e = dynamic_cast<Expression*>(&*m);
    if (e == nullptr)
        throw Phrase_Error(ph, "not an expression");
    return Shared<Expression>(e);
}

Shared<Expression>
Environ::lookup(const Identifier& id)
{
    for (Environ* e = this; e != nullptr; e = e->parent_) {
        auto m = e->single_lookup(id);
        if (m != nullptr)
            return m;
    }
    throw Phrase_Error(id, stringify(id.atom_,": not defined"));
}

Shared<Expression>
Builtin_Environ::single_lookup(const Identifier& id)
{
    auto p = names.find(id.atom_);
    if (p != names.end())
        return aux::make_shared<Constant>(
            Shared<const Phrase>(&id), p->second);
    return nullptr;
}

Shared<Expression>
Bindings::Environ::single_lookup(const Identifier& id)
{
    auto b = bindings_.dictionary_->find(id.atom_);
    if (b != bindings_.dictionary_->end())
        return aux::make_shared<Module_Ref>(Shared<const Phrase>(&id), b->second);
    return nullptr;
}

Shared<Meaning>
Identifier::analyze(Environ& env) const
{
    return env.lookup(*this);
}

Shared<Meaning>
Numeral::analyze(Environ& env) const
{
    std::string str(location().range());
    char* endptr;
    double n = strtod(str.c_str(), &endptr);
    assert(endptr == str.c_str() + str.size());
    return aux::make_shared<Constant>(Shared<const Phrase>(this), n);
}
Shared<Meaning>
String_Phrase::analyze(Environ& env) const
{
    auto str = location().range();
    assert(str.size() >= 2); // includes start and end " characters
    assert(*str.begin() == '"');
    assert(*(str.begin()+str.size()-1) == '"');
    ++str.first;
    --str.last;
    return aux::make_shared<Constant>(Shared<const Phrase>(this),
        Value{String::make(str.begin(),str.size())});
}

Shared<Meaning>
Unary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind) {
    case Token::k_not:
        return aux::make_shared<Not_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*arg_, env));
    default:
        return aux::make_shared<Prefix_Expr>(
            Shared<const Phrase>(this),
            op_.kind,
            curv::analyze_expr(*arg_, env));
    }
}

Shared<Meaning>
Lambda_Phrase::analyze(Environ& env) const
{
    // Syntax: id->expr or (a,b,...)->expr
    // TODO: pattern matching: [a,b]->expr, {a,b}->expr

    // phase 1: Create a dictionary of parameters.
    Atom_Map<int> params;
    int slot = 0;
    if (auto id = dynamic_cast<const Identifier*>(left_.get()))
    {
        Atom name = id->location().range();
        params[name] = slot++;
    }
    else if (auto parens = dynamic_cast<const Paren_Phrase*>(left_.get()))
    {
        for (auto a : parens->args_) {
            if (auto id = dynamic_cast<const Identifier*>(a.expr_.get())) {
                Atom name = id->location().range();
                params[name] = slot++;
            } else
                throw Phrase_Error(*a.expr_, "not a parameter");
        }
    }
    else
        throw Phrase_Error(*left_, "not a parameter");

    // Phase 2: make an Environ from the parameters and analyze the body.
    struct Arg_Environ : public Environ
    {
        Atom_Map<int>& names_;
        Module::Dictionary nonlocal_dictionary_;
        std::vector<Shared<const Expression>> nonlocal_exprs_;
        bool recursive_;

        Arg_Environ(Environ* parent, Atom_Map<int>& names, bool recursive)
        : Environ(parent), names_(names), recursive_(recursive)
        {
            frame_nslots = names.size();
            frame_maxslots = names.size();
        }
        virtual Shared<Expression> single_lookup(const Identifier& id)
        {
            auto p = names_.find(id.atom_);
            if (p != names_.end())
                return aux::make_shared<Arg_Ref>(
                    Shared<const Phrase>(&id), p->second);
            if (recursive_)
                return nullptr;
            // In non-recursive mode, we return a definitive result.
            // We don't return nullptr (meaning try again in parent_).
            auto n = nonlocal_dictionary_.find(id.atom_);
            if (n != nonlocal_dictionary_.end()) {
                return aux::make_shared<Nonlocal_Ref>(
                    Shared<const Phrase>(&id), n->second);
            }
            auto m = parent_->lookup(id);
            if (dynamic_cast<Constant*>(m.get()) != nullptr)
                return m;
            size_t slot = nonlocal_exprs_.size();
            nonlocal_dictionary_[id.atom_] = slot;
            nonlocal_exprs_.push_back(m);
            return aux::make_shared<Nonlocal_Ref>(
                Shared<const Phrase>(&id), slot);
        }
    };
    Arg_Environ env2(&env, params, recursive_);
    auto expr = curv::analyze_expr(*right_, env2);
    auto src = Shared<const Lambda_Phrase>(this);
    Shared<List_Expr> nonlocals =
        List_Expr::make(env2.nonlocal_exprs_.size(), src);
    // TODO: use some kind of Tail_Array move constructor
    for (size_t i = 0; i < env2.nonlocal_exprs_.size(); ++i)
        (*nonlocals)[i] = env2.nonlocal_exprs_[i];

    return aux::make_shared<Lambda_Expr>(
        src, expr, nonlocals, params.size(), env2.frame_maxslots);
}

Shared<Meaning>
Binary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind) {
    case Token::k_or:
        return aux::make_shared<Or_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_and:
        return aux::make_shared<And_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_equal:
        return aux::make_shared<Equal_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_not_equal:
        return aux::make_shared<Not_Equal_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_less:
        return aux::make_shared<Less_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_greater:
        return aux::make_shared<Greater_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_less_or_equal:
        return aux::make_shared<Less_Or_Equal_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_greater_or_equal:
        return aux::make_shared<Greater_Or_Equal_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_power:
        return aux::make_shared<Power_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    case Token::k_at:
        return aux::make_shared<At_Expr>(
            Shared<const Phrase>(this),
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    default:
        return aux::make_shared<Infix_Expr>(
            Shared<const Phrase>(this),
            op_.kind,
            curv::analyze_expr(*left_, env),
            curv::analyze_expr(*right_, env));
    }
}

Shared<Meaning>
Definition::analyze(Environ& env) const
{
    throw Phrase_Error(*this, "not an expression");
}

Shared<Meaning>
Paren_Phrase::analyze(Environ& env) const
{
    if (args_.size() == 1
        && args_[0].comma_.kind == Token::k_missing)
    {
        return curv::analyze_expr(*args_[0].expr_, env);
    } else {
        throw Phrase_Error(*this, "not an expression");
    }
}

Shared<Meaning>
List_Phrase::analyze(Environ& env) const
{
    Shared<List_Expr> list =
        List_Expr::make(args_.size(), Shared<const Phrase>(this));
    for (size_t i = 0; i < args_.size(); ++i)
        (*list)[i] = analyze_expr(*args_[i].expr_, env);
    return list;
}

Shared<Meaning>
Call_Phrase::analyze(Environ& env) const
{
    auto fun = curv::analyze_expr(*function_, env);
    std::vector<Shared<Expression>> args;

    auto patom = dynamic_cast<Paren_Phrase*>(&*args_);
    if (patom != nullptr) {
        // argument phrase is a variable-length parenthesized argument list
        args.reserve(patom->args_.size());
        for (auto a : patom->args_)
            args.push_back(curv::analyze_expr(*a.expr_, env));
    } else {
        // argument phrase is a unitary expression
        args.reserve(1);
        args.push_back(curv::analyze_expr(*args_, env));
    }

    return aux::make_shared<Call_Expr>(
        Shared<const Phrase>(this),
        std::move(fun),
        args_,
        std::move(args));
}

Shared<Meaning>
Dot_Phrase::analyze(Environ& env) const
{
    return aux::make_shared<Dot_Expr>(
        Shared<const Phrase>(this),
        curv::analyze_expr(*left_, env),
        id_->atom_);
}

void
analyze_definition(
    const Definition& def,
    Atom_Map<Shared<const Expression>>& dict,
    Environ& env)
{
    auto id = dynamic_cast<const Identifier*>(def.left_.get());
    if (id == nullptr)
        throw Phrase_Error(*def.left_, "not an identifier");
    Atom name = id->location().range();
    if (dict.find(name) != dict.end())
        throw Phrase_Error(*def.left_, stringify(name, ": multiply defined"));
    dict[name] = curv::analyze_expr(*def.right_, env);
}

Shared<Meaning>
Module_Phrase::analyze(Environ& env) const
{
    return analyze_module(env);
}

void
Bindings::add_definition(Shared<Phrase> phrase)
{
    const Definition* def = dynamic_cast<Definition*>(phrase.get());
    if (def == nullptr)
        throw Phrase_Error(*phrase, "not a definition");
    auto id = dynamic_cast<const Identifier*>(def->left_.get());
    if (id == nullptr)
        throw Phrase_Error(*def->left_, "not an identifier");
    Atom name = id->location().range();
    if (dictionary_->find(name) != dictionary_->end())
        throw Phrase_Error(*def->left_,
            stringify(name, ": multiply defined"));
    (*dictionary_)[name] = cur_position_++;
    slot_phrases_.push_back(def->right_);
}

Shared<List>
Bindings::analyze_values(Environ& env)
{
    size_t n = slot_phrases_.size();
    auto slots = make_list(n);
    for (size_t i = 0; i < n; ++i) {
        auto expr = curv::analyze_expr(*slot_phrases_[i], env);
        (*slots)[i] = {aux::make_shared<Thunk>(expr)};
    }
    return slots;
}

Shared<Module_Expr>
Module_Phrase::analyze_module(Environ& env) const
{
    // phase 1: Create a dictionary of field phrases, a list of element phrases
    Bindings fields;
    std::vector<Shared<Phrase>> elements;
    for (auto st : stmts_) {
        if (dynamic_cast<Definition*>(st.stmt_.get()) != nullptr)
            fields.add_definition(st.stmt_);
        else
            elements.push_back(st.stmt_);
    }

    // phase 2: Construct an environment from the field dictionary
    // and use it to perform semantic analysis.
    Bindings::Environ env2(&env, fields);
    auto self = Shared<const Phrase>(this);
    auto module = aux::make_shared<Module_Expr>(self);
    module->dictionary_ = fields.dictionary_;
    module->slots_ = fields.analyze_values(env2);
    Shared<List_Expr> xelements = {List_Expr::make(elements.size(), self)};
    for (size_t i = 0; i < elements.size(); ++i)
        (*xelements)[i] = curv::analyze_expr(*elements[i], env2);
    module->elements_ = xelements;
    module->frame_nslots_ = env2.frame_maxslots;
    return module;
}

Shared<Meaning>
Record_Phrase::analyze(Environ& env) const
{
    Shared<Record_Expr> record =
        aux::make_shared<Record_Expr>(Shared<const Phrase>(this));
    for (auto i : args_) {
        const Definition* def =
            dynamic_cast<Definition*>(i.expr_.get());
        if (def != nullptr) {
            analyze_definition(*def, record->fields_, env);
        } else {
            throw Phrase_Error(*i.expr_, "not a definition");
        }
    }
    return record;
}

Shared<Meaning>
If_Phrase::analyze(Environ& env) const
{
    return aux::make_shared<If_Expr>(
        Shared<const Phrase>(this),
        curv::analyze_expr(*condition_, env),
        curv::analyze_expr(*then_expr_, env),
        curv::analyze_expr(*else_expr_, env));
}

Shared<Meaning>
Let_Phrase::analyze(Environ& env) const
{
    // `let` supports mutually recursive bindings, like let-rec in Scheme.
    //
    // To evaluate: lazy evaluation of thunks, error on illegal recursion.
    // These thunks do not get a fresh evaluation Frame, they use the Frame
    // of the `let` expression. That's consistent with an optimizing compiler
    // where let bindings are SSA values.
    //
    // To analyze: we assign a slot number to each of the let bindings,
    // *then* we construct an Environ and analyze each definiens.
    // This means no opportunity for optimization (eg, don't store a let binding
    // in a slot or create a Thunk if it is a Constant). To optimize, we need
    // another compiler pass or two, so that we do register allocation *after*
    // analysis and constant folding is complete.

    // phase 1: Create a dictionary of bindings.
    struct Binding
    {
        int slot_;
        Shared<Phrase> phrase_;
        Binding(int slot, Shared<Phrase> phrase)
        : slot_(slot), phrase_(phrase)
        {}
        Binding(){}
    };
    Atom_Map<Binding> bindings;
    int slot = env.frame_nslots;
    for (auto b : bindings_->args_) {
        const Definition* def = dynamic_cast<Definition*>(b.expr_.get());
        if (def == nullptr)
            throw Phrase_Error(*b.expr_, "not a definition");
        auto id = dynamic_cast<const Identifier*>(def->left_.get());
        if (id == nullptr)
            throw Phrase_Error(*def->left_, "not an identifier");
        Atom name = id->location().range();
        if (bindings.find(name) != bindings.end())
            throw Phrase_Error(*def->left_,
                stringify(name, ": multiply defined"));
        bindings[name] = Binding{slot++, def->right_};
    }

    // phase 2: Construct an environment from the bindings dictionary
    // and use it to perform semantic analysis.
    struct Let_Environ : public Environ
    {
    protected:
        const Atom_Map<Binding>& names;
    public:
        Let_Environ(
            Environ* p,
            const Atom_Map<Binding>& n)
        : Environ(p), names(n)
        {
            if (p != nullptr) {
                frame_nslots = p->frame_nslots;
                frame_maxslots = p->frame_maxslots;
            }
            frame_nslots += n.size();
            frame_maxslots = std::max(frame_maxslots, frame_nslots);
        }
        virtual Shared<Expression> single_lookup(const Identifier& id)
        {
            auto p = names.find(id.atom_);
            if (p != names.end())
                return aux::make_shared<Let_Ref>(
                    Shared<const Phrase>(&id), p->second.slot_);
            return nullptr;
        }
    };
    Let_Environ env2(&env, bindings);

    int first_slot = env.frame_nslots;
    std::vector<Value> values(bindings.size());
    for (auto b : bindings) {
        auto expr = curv::analyze_expr(*b.second.phrase_, env2);
        values[b.second.slot_-first_slot] = {aux::make_shared<Thunk>(expr)};
    }
    auto body = curv::analyze_expr(*body_, env2);
    env.frame_maxslots = env2.frame_maxslots;
    assert(env.frame_maxslots >= bindings.size());

    return aux::make_shared<Let_Expr>(Shared<const Phrase>(this),
        first_slot, std::move(values), body);
}

} // namespace curv
