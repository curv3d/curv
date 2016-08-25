// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/shared.h>
#include <curv/exception.h>
#include <curv/thunk.h>
using namespace curv;

Shared<Expression>
curv::analyze_expr(Phrase& ph, Environ& env)
{
    Shared<Meaning> m = analyze(ph, env);
    Expression* e = dynamic_cast<Expression*>(&*m);
    if (e == nullptr)
        throw Phrase_Error(ph, "not an expression");
    return Shared<Expression>(e);
}

Shared<Meaning>
curv::Environ::lookup(const Identifier& id) const
{
    Atom a(id.location().range());
    for (const Environ* e = this; e != nullptr; e = e->parent) {
        auto m = e->single_lookup(id, a);
        if (m != nullptr)
            return m;
    }
    throw Phrase_Error(id, stringify(a,": not defined"));
}

Shared<Meaning>
curv::Builtin_Environ::single_lookup(const Identifier& id, Atom a) const
{
    auto p = names.find(a);
    if (p != names.end())
        return aux::make_shared<Constant>(
            Shared<const Phrase>(&id), p->second);
    return nullptr;
}

Shared<Meaning>
curv::Module_Environ::single_lookup(const Identifier& id, Atom a) const
{
    auto p = names.find(a);
    if (p != names.end())
        return aux::make_shared<Module_Ref>(
            Shared<const Phrase>(&id), a);
    return nullptr;
}

Shared<Meaning>
curv::Identifier::analyze(Environ& env) const
{
    return env.lookup(*this);
}

Shared<Meaning>
curv::Numeral::analyze(Environ& env) const
{
    std::string str(location().range());
    char* endptr;
    double n = strtod(str.c_str(), &endptr);
    assert(endptr == str.c_str() + str.size());
    return aux::make_shared<Constant>(Shared<const Phrase>(this), n);
}
Shared<Meaning>
curv::String_Phrase::analyze(Environ& env) const
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
curv::Unary_Phrase::analyze(Environ& env) const
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
curv::Binary_Phrase::analyze(Environ& env) const
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
curv::Definition::analyze(Environ& env) const
{
    throw Phrase_Error(*this, "not an expression");
}

Shared<Meaning>
curv::Paren_Phrase::analyze(Environ& env) const
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
curv::List_Phrase::analyze(Environ& env) const
{
    Shared<List_Expr> list =
        List_Expr::make(args_.size(), Shared<const Phrase>(this));
    for (size_t i = 0; i < args_.size(); ++i)
        (*list)[i] = analyze_expr(*args_[i].expr_, env);
    return list;
}

Shared<Meaning>
curv::Call_Phrase::analyze(Environ& env) const
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
curv::Dot_Phrase::analyze(Environ& env) const
{
    return aux::make_shared<Dot_Expr>(
        Shared<const Phrase>(this),
        curv::analyze_expr(*left_, env),
        id_->make_atom());
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
curv::Module_Phrase::analyze(Environ& env) const
{
    return analyze_module(env);
}

Shared<Module_Expr>
curv::Module_Phrase::analyze_module(Environ& env) const
{
    // phase 1: Create a dictionary of field phrases, a list of element phrases
    Atom_Map<Shared<Phrase>> fields;
    std::vector<Shared<Phrase>> elements;
    for (auto st : stmts_) {
        const Definition* def =
            dynamic_cast<Definition*>(st.stmt_.get());
        if (def != nullptr) {
            auto id = dynamic_cast<const Identifier*>(def->left_.get());
            if (id == nullptr)
                throw Phrase_Error(*def->left_, "not an identifier");
            Atom name = id->location().range();
            if (fields.find(name) != fields.end())
                throw Phrase_Error(*def->left_,
                    stringify(name, ": multiply defined"));
            fields[name] = def->right_;
        } else {
            elements.push_back(st.stmt_);
        }
    }

    // phase 2: Construct an environment from the field dictionary
    // and use it to perform semantic analysis.
    Module_Environ env2(&env, fields);
    auto self = Shared<const Phrase>(this);
    auto module = aux::make_shared<Module_Expr>(self);
    for (auto f : fields)
        module->fields_[f.first] = curv::analyze_expr(*f.second, env2);
    auto xelements = List_Expr::make(elements.size(), self);
    for (size_t i = 0; i < elements.size(); ++i)
        (*xelements)[i] = curv::analyze_expr(*elements[i], env2);
    module->elements_ = xelements;
    module->frame_nslots_ = env2.frame_maxslots;
    return module;
}

Shared<Meaning>
curv::Record_Phrase::analyze(Environ& env) const
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
curv::If_Phrase::analyze(Environ& env) const
{
    return aux::make_shared<If_Expr>(
        Shared<const Phrase>(this),
        curv::analyze_expr(*condition_, env),
        curv::analyze_expr(*then_expr_, env),
        curv::analyze_expr(*else_expr_, env));
}

Shared<Meaning>
curv::Let_Phrase::analyze(Environ& env) const
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
        virtual Shared<Meaning> single_lookup(const Identifier& id, Atom a) const
        {
            auto p = names.find(a);
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
