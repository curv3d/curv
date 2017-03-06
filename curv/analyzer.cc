// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <functional>

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/shared.h>
#include <curv/exception.h>
#include <curv/thunk.h>
#include <curv/function.h>
#include <curv/context.h>

namespace curv
{

Shared<Operation>
analyze_op(const Phrase& ph, Environ& env)
{
    return ph.analyze(env)->to_operation(env);
}

Shared<Operation>
Metafunction::to_operation(Environ& env)
{
    throw Exception(At_Phrase(*source_, env), "not an operation");
}

Shared<Operation>
Operation::to_operation(Environ&)
{
    return share(*this);
}

Shared<Definition>
Phrase::analyze_def(Environ&) const
{
    return nullptr;
}

Shared<Meaning>
Environ::lookup(const Identifier& id)
{
    for (Environ* e = this; e != nullptr; e = e->parent_) {
        auto m = e->single_lookup(id);
        if (m != nullptr)
            return m;
    }
    throw Exception(At_Phrase(id, *this), stringify(id.atom_,": not defined"));
}

Shared<Meaning>
Builtin_Environ::single_lookup(const Identifier& id)
{
    auto p = names.find(id.atom_);
    if (p != names.end())
        return p->second->to_meaning(id);
    return nullptr;
}

void
Bindings::add_definition(Shared<Definition> def, curv::Environ& env)
{
    Atom name = def->name_->atom_;
    if (dictionary_->find(name) != dictionary_->end())
        throw Exception(At_Phrase(*def->name_, env),
            stringify(name, ": multiply defined"));
    (*dictionary_)[name] = cur_position_++;
    slot_phrases_.push_back(def->definiens_);

    auto lambda = dynamic_cast<Lambda_Phrase*>(def->definiens_.get());
    if (lambda != nullptr)
        lambda->recursive_ = true;
}

bool
Bindings::is_recursive_function(size_t slot)
{
    return isa_shared<const Lambda_Phrase>(slot_phrases_[slot]);
}

Shared<Meaning>
Bindings::Environ::single_lookup(const Identifier& id)
{
    auto b = bindings_.dictionary_->find(id.atom_);
    if (b != bindings_.dictionary_->end()) {
        if (bindings_.is_recursive_function(b->second))
            return make<Nonlocal_Function_Ref>(
                share(id), b->second);
        else
            return make<Module_Ref>(
                share(id), b->second);
    }
    return nullptr;
}

Shared<List>
Bindings::analyze_values(Environ& env)
{
    size_t n = slot_phrases_.size();
    auto slots = make_list(n);
    for (size_t i = 0; i < n; ++i) {
        auto expr = analyze_op(*slot_phrases_[i], env);
        if (is_recursive_function(i)) {
            auto& l = dynamic_cast<Lambda_Expr&>(*expr);
            (*slots)[i] = {make<Lambda>(l.body_,l.nargs_,l.nslots_)};
        } else
            (*slots)[i] = {make<Thunk>(expr)};
    }
    return slots;
}

Shared<Meaning>
Empty_Phrase::analyze(Environ& env) const
{
    return make<Null_Action>(share(*this));
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
    return make<Constant>(share(*this), n);
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
    return make<Constant>(share(*this),
        Value{String::make(str.begin(),str.size())});
}

Shared<Meaning>
Unary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind) {
    case Token::k_not:
        return make<Not_Expr>(
            share(*this),
            analyze_op(*arg_, env));
    case Token::k_plus:
        return make<Positive_Expr>(
            share(*this),
            analyze_op(*arg_, env));
    case Token::k_minus:
        return make<Negative_Expr>(
            share(*this),
            analyze_op(*arg_, env));
    case Token::k_ellipsis:
        return make<Spread_Gen>(
            share(*this),
            analyze_op(*arg_, env));
    default:
        assert(0);
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
    each_argument(*left_, [&](const Phrase& p)->void {
        if (auto id = dynamic_cast<const Identifier*>(&p))
            params[id->atom_] = slot++;
        else
            throw Exception(At_Phrase(p, env), "not a parameter");
    });

    // Phase 2: make an Environ from the parameters and analyze the body.
    struct Arg_Environ : public Environ
    {
        Atom_Map<int>& names_;
        Module::Dictionary nonlocal_dictionary_;
        std::vector<Shared<const Operation>> nonlocal_exprs_;
        bool recursive_;

        Arg_Environ(Environ* parent, Atom_Map<int>& names, bool recursive)
        : Environ(parent), names_(names), recursive_(recursive)
        {
            frame_nslots = names.size();
            frame_maxslots = names.size();
        }
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            auto p = names_.find(id.atom_);
            if (p != names_.end())
                return make<Arg_Ref>(
                    share(id), p->second);
            if (recursive_)
                return nullptr;
            // In non-recursive mode, we return a definitive result.
            // We don't return nullptr (meaning try again in parent_).
            auto n = nonlocal_dictionary_.find(id.atom_);
            if (n != nonlocal_dictionary_.end()) {
                return make<Nonlocal_Ref>(
                    share(id), n->second);
            }
            auto m = parent_->lookup(id);
            if (isa_shared<Constant>(m))
                return m;
            if (auto expr = dynamic_shared_cast<Operation>(m)) {
                size_t slot = nonlocal_exprs_.size();
                nonlocal_dictionary_[id.atom_] = slot;
                nonlocal_exprs_.push_back(expr);
                return make<Nonlocal_Ref>(
                    share(id), slot);
            }
            return m;
        }
    };
    Arg_Environ env2(&env, params, recursive_);
    auto expr = analyze_op(*right_, env2);
    auto src = share(*this);
    Shared<List_Expr> nonlocals =
        List_Expr::make(env2.nonlocal_exprs_.size(), src);
    // TODO: use some kind of Tail_Array move constructor
    for (size_t i = 0; i < env2.nonlocal_exprs_.size(); ++i)
        (*nonlocals)[i] = env2.nonlocal_exprs_[i];

    return make<Lambda_Expr>(
        src, expr, nonlocals, params.size(), env2.frame_maxslots);
}

Shared<Meaning>
Binary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind) {
    case Token::k_or:
        return make<Or_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_and:
        return make<And_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_equal:
        return make<Equal_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_not_equal:
        return make<Not_Equal_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_less:
        return make<Less_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_greater:
        return make<Greater_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_less_or_equal:
        return make<Less_Or_Equal_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_greater_or_equal:
        return make<Greater_Or_Equal_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_plus:
        return make<Add_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_minus:
        return make<Subtract_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_times:
        return make<Multiply_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_over:
        return make<Divide_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_power:
        return make<Power_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_dot:
        if (auto id = dynamic_shared_cast<Identifier>(right_))
            return make<Dot_Expr>(
                share(*this),
                analyze_op(*left_, env),
                id->atom_);
        throw Exception(At_Phrase(*right_, env),
            "invalid expression after '.'");
    case Token::k_apostrophe:
        return make<At_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    default:
        assert(0);
    }
}

Shared<Meaning>
Definition_Phrase::analyze(Environ& env) const
{
    throw Exception(At_Phrase(*this, env), "not an operation");
}

Shared<Definition>
analyze_def_iter(Environ& env, Phrase& left, Shared<Phrase> right)
{
    if (auto id = dynamic_cast<const Identifier*>(&left))
        return make<Definition>(share(*id), right);
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return analyze_def_iter(env, *call->function_,
            make<Lambda_Phrase>(call->args_, Token(), right));
    throw Exception(At_Phrase(left,  env), "invalid definiendum");
}

Shared<Definition>
Definition_Phrase::analyze_def(Environ& env) const
{
    return analyze_def_iter(env, *left_, right_);
}

Shared<Meaning>
Semicolon_Phrase::analyze(Environ& env) const
{
    auto block = Block_Op::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        (*block)[i] = analyze_op(*args_[i].expr_, env);
    return block;
}

Shared<Meaning>
Comma_Phrase::analyze(Environ& env) const
{
    auto seq = Sequence_Gen::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        (*seq)[i] = analyze_op(*args_[i].expr_, env);
    return seq;
}

Shared<Meaning>
Paren_Phrase::analyze(Environ& env) const
{
    if (dynamic_shared_cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        auto list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyze_op(*items[i].expr_, env);
        return list;
    } else
        return analyze_op(*body_, env);
}

Shared<Meaning>
List_Phrase::analyze(Environ& env) const
{
    if (dynamic_shared_cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        auto list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyze_op(*items[i].expr_, env);
        return list;
    } else {
        auto list = List_Expr::make(1, share(*this));
        (*list)[0] = analyze_op(*body_, env);
        return list;
    }
}

Shared<Meaning>
Call_Phrase::analyze(Environ& env) const
{
    return function_->analyze(env)->call(*this, env);
}

Shared<Meaning>
Operation::call(const Call_Phrase& src, Environ& env)
{
    auto argv = src.analyze_args(env);
    return make<Call_Expr>(
        share(src),
        share(*this),
        std::move(argv));
}

std::vector<Shared<Operation>>
Call_Phrase::analyze_args(Environ& env) const
{
    std::vector<Shared<Operation>> argv;
    each_argument(*args_, [&](const Phrase& p)->void {
        argv.push_back(analyze_op(p, env));
    });
    return std::move(argv);
}

void
analyze_definition(
    const Definition& def,
    Atom_Map<Shared<const Operation>>& dict,
    Environ& env)
{
    Atom name = def.name_->atom_;
    if (dict.find(name) != dict.end())
        throw Exception(At_Phrase(*def.name_, env),
            stringify(name, ": multiply defined"));
    dict[name] = analyze_op(*def.definiens_, env);
}

Shared<Meaning>
Module_Phrase::analyze(Environ& env) const
{
    return analyze_module(env);
}

/// In the grammar, a <semicolons> phrase is one or more constituent phrases
/// separated by semicolons. This function iterates over each constituent
/// phrase.
static inline void
each_statement(const Phrase& phrase, std::function<void(const Phrase&)> func)
{
    if (dynamic_cast<const Empty_Phrase*>(&phrase))
        return;
    if (auto semis = dynamic_cast<const Semicolon_Phrase*>(&phrase)) {
        for (auto& i : semis->args_)
            func(*i.expr_);
    } else {
        func(phrase);
    }
}

Shared<Module_Expr>
Module_Phrase::analyze_module(Environ& env) const
{
    // phase 1: Create a dictionary of field phrases, a list of element phrases
    Bindings fields;
    std::vector<Shared<const Phrase>> elements;
    each_statement(*body_, [&](const Phrase& stmt)->void {
        auto def = stmt.analyze_def(env);
        if (def != nullptr)
            fields.add_definition(def, env);
        else
            elements.push_back(share(stmt));
    });

    // phase 2: Construct an environment from the field dictionary
    // and use it to perform semantic analysis.
    Bindings::Environ env2(&env, fields);
    auto self = share(*this);
    auto module = make<Module_Expr>(self);
    module->dictionary_ = fields.dictionary_;
    module->slots_ = fields.analyze_values(env2);
    Shared<List_Expr> xelements = {List_Expr::make(elements.size(), self)};
    for (size_t i = 0; i < elements.size(); ++i)
        (*xelements)[i] = analyze_op(*elements[i], env2);
    module->elements_ = xelements;
    module->frame_nslots_ = env2.frame_maxslots;
    return module;
}

/// In the grammar, a <commas> phrase is one or more constituent phrases
/// separated by commas. This function iterates over each constituent phrase.
static inline void
each_item(const Phrase& phrase, std::function<void(const Phrase&)> func)
{
    if (dynamic_cast<const Empty_Phrase*>(&phrase))
        return;
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&phrase)) {
        for (auto& i : commas->args_)
            func(*i.expr_);
    } else {
        func(phrase);
    }
}

Shared<Meaning>
Brace_Phrase::analyze(Environ& env) const
{
    auto record = make<Record_Expr>(share(*this));
    each_item(*body_, [&](const Phrase& item)->void {
        if (auto def = item.analyze_def(env))
            analyze_definition(*def, record->fields_, env);
        else
            throw Exception(At_Phrase(item, env), "not a definition");
    });
    return record;
}

Shared<Meaning>
If_Phrase::analyze(Environ& env) const
{
    if (else_expr_ == nullptr) {
        return make<If_Op>(
            share(*this),
            analyze_op(*condition_, env),
            analyze_op(*then_expr_, env));
    } else {
        return make<If_Else_Op>(
            share(*this),
            analyze_op(*condition_, env),
            analyze_op(*then_expr_, env),
            analyze_op(*else_expr_, env));
    }
}

Shared<Meaning>
Let_Phrase::analyze(Environ& env) const
{
    // `let` binds names in the parent scope, like `let` in Scheme.

    int first_slot = env.frame_nslots;
    std::vector<Shared<Operation>> exprs;
    Atom_Map<int> names;

    int slot = first_slot;
    each_argument(*args_, [&](const Phrase& p)->void {
        auto def = p.analyze_def(env);
        if (def == nullptr)
            throw Exception(At_Phrase(p, env), "not a definition");
        Atom name = def->name_->atom_;
        if (names.find(name) != names.end())
            throw Exception(At_Phrase(*def->name_, env),
                stringify(name, ": multiply defined"));
        names[name] = slot++;

        exprs.push_back(analyze_op(*def->definiens_, env));

        // This is subtle: it works because the expressions in `exprs`
        // are evaluated from first to last while populating the slots owned
        // by the `let` expression in the same order.
        ++env.frame_nslots;
        env.frame_maxslots = std::max(env.frame_maxslots, env.frame_nslots);
    });
    env.frame_nslots = first_slot;

    struct Let_Environ : public Environ
    {
    protected:
        const Atom_Map<int>& names;
    public:
        Let_Environ(
            Environ* p,
            const Atom_Map<int>& n)
        : Environ(p), names(n)
        {
            if (p != nullptr) {
                frame_nslots = p->frame_nslots;
                frame_maxslots = p->frame_maxslots;
            }
            frame_nslots += n.size();
            frame_maxslots = std::max(frame_maxslots, frame_nslots);
        }
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            auto p = names.find(id.atom_);
            if (p != names.end())
                return make<Let_Ref>(share(id), p->second);
            return nullptr;
        }
    };
    Let_Environ env2(&env, names);

    auto body = analyze_op(*body_, env2);
    env.frame_maxslots = env2.frame_maxslots;
    assert(env.frame_maxslots >= names.size());

    return make<Let_Op>(share(*this),
        first_slot, std::move(exprs), body);
}

Shared<Meaning>
Letrec_Phrase::analyze(Environ& env) const
{
    // `letrec` supports mutually recursive bindings, like let-rec in Scheme.
    //
    // To evaluate: lazy evaluation of thunks, error on illegal recursion.
    // These thunks do not get a fresh evaluation Frame, they use the Frame
    // of the `letrec` expression. That's consistent with an optimizing compiler
    // where letrec bindings are SSA values.
    //
    // To analyze: we assign a slot number to each of the letrec bindings,
    // *then* we construct an Environ and analyze each definiens.
    // This means no opportunity for optimization (eg, don't store a letrec binding
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
    each_argument(*args_, [&](const Phrase& p)->void {
        auto def = p.analyze_def(env);
        if (def == nullptr)
            throw Exception(At_Phrase(p, env), "not a definition");
        Atom name = def->name_->atom_;
        if (bindings.find(name) != bindings.end())
            throw Exception(At_Phrase(*def->name_, env),
                stringify(name, ": multiply defined"));
        bindings[name] = Binding{slot++, def->definiens_};
    });

    // phase 2: Construct an environment from the bindings dictionary
    // and use it to perform semantic analysis.
    struct Letrec_Environ : public Environ
    {
    protected:
        const Atom_Map<Binding>& names;
    public:
        Letrec_Environ(
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
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            auto p = names.find(id.atom_);
            if (p != names.end())
                return make<Letrec_Ref>(
                    share(id), p->second.slot_);
            return nullptr;
        }
    };
    Letrec_Environ env2(&env, bindings);

    int first_slot = env.frame_nslots;
    std::vector<Value> values(bindings.size());
    for (auto b : bindings) {
        auto expr = analyze_op(*b.second.phrase_, env2);
        values[b.second.slot_-first_slot] = {make<Thunk>(expr)};
    }
    auto body = analyze_op(*body_, env2);
    env.frame_maxslots = env2.frame_maxslots;
    assert(env.frame_maxslots >= bindings.size());

    return make<Letrec_Op>(share(*this),
        first_slot, std::move(values), body);
}

Shared<Meaning>
For_Phrase::analyze(Environ& env) const
{
    auto defexpr = args_->body_;
    const Definition_Phrase* def = dynamic_cast<Definition_Phrase*>(&*defexpr);
    if (def == nullptr)
        throw Exception(At_Phrase(*args_, env),
            "for: malformed argument");
    auto id = dynamic_cast<const Identifier*>(def->left_.get());
    if (id == nullptr)
        throw Exception(At_Phrase(*def->left_, env), "for: not an identifier");
    Atom name = id->atom_;

    auto list = analyze_op(*def->right_, env);

    int slot = env.frame_nslots;
    struct For_Environ : public Environ
    {
        Atom name_;
        int slot_;

        For_Environ(
            Environ& p,
            Atom name,
            int slot)
        : Environ(&p), name_(name), slot_(slot)
        {
            frame_nslots = p.frame_nslots + 1;
            frame_maxslots = std::max(p.frame_maxslots, frame_nslots);
        }
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            if (id.atom_ == name_)
                return make<Letrec_Ref>(
                    share(id), slot_);
            return nullptr;
        }
    };
    For_Environ body_env(env, name, slot);
    auto body = analyze_op(*body_, body_env);
    env.frame_maxslots = body_env.frame_maxslots;

    return make<For_Op>(share(*this), slot, list, body);
}

Shared<Meaning>
Range_Phrase::analyze(Environ& env) const
{
    return make<Range_Expr>(
        share(*this),
        analyze_op(*first_, env),
        analyze_op(*last_, env),
        step_ ? analyze_op(*step_, env) : nullptr,
        op1_.kind == Token::k_open_range);
}

} // namespace curv
