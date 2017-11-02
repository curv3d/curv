// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <functional>

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/definition.h>
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
    bool old = env.is_analyzing_action_;
    env.is_analyzing_action_ = false;
    auto result = ph.analyze(env)->to_operation(env.eval_frame_);
    env.is_analyzing_action_ = old;
    return result;
}

Shared<Operation>
analyze_action(const Phrase& ph, Environ& env)
{
    bool old = env.is_analyzing_action_;
    env.is_analyzing_action_ = true;
    auto result = ph.analyze(env)->to_operation(env.eval_frame_);
    env.is_analyzing_action_ = old;
    return result;
}

Shared<Operation>
analyze_tail(const Phrase& ph, Environ& env)
{
    return ph.analyze(env)->to_operation(env.eval_frame_);
}

Shared<Operation>
Meaning::to_operation(Frame* f)
{
    throw Exception(At_Phrase(*source_, f), "not an operation");
}

Shared<Meaning>
Meaning::call(const Call_Phrase&, Environ& env)
{
    throw Exception(At_Phrase(*source_, env), "not callable");
}

Shared<Operation>
Operation::to_operation(Frame*)
{
    return share(*this);
}

Shared<Definition>
Phrase::as_definition(Environ&)
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
Environ::lookup_var(const Identifier& id)
{
    for (Environ* e = this; e != nullptr; e = e->parent_) {
        if (!e->is_analyzing_action_)
            break;
        if (e->is_sequential_statement_list_) {
            auto m = e->single_lookup(id);
            if (m != nullptr)
                return m;
        }
    }
    throw Exception(At_Phrase(id, *this),
        stringify("var ",id.atom_,": not defined"));
}

Shared<Meaning>
Builtin_Environ::single_lookup(const Identifier& id)
{
    auto p = names.find(id.atom_);
    if (p != names.end())
        return p->second->to_meaning(id);
    return nullptr;
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
    switch (loc_.token().kind_) {
    case Token::k_num:
      {
        std::string str(location().range());
        char* endptr;
        double n = strtod(str.c_str(), &endptr);
        assert(endptr == str.c_str() + str.size());
        return make<Constant>(share(*this), n);
      }
    case Token::k_hexnum:
      {
        double n = 0.0;
        auto numeral = location().range();
        for (const char* p = numeral.first+2; p < numeral.last; ++p) {
            char d = *p;
            if (d >= '0' && d <= '9')
                n = 16.0*n + (d-'0');
            else if (d >= 'a' && d <= 'f')
                n = 16.0*n + (d-'a'+10);
            else if (d >= 'A' && d <= 'F')
                n = 16.0*n + (d-'A'+10);
            else
                assert(0);
        }
        return make<Constant>(share(*this), n);
      }
    default:
        assert(0);
    }
}

Shared<Segment>
String_Segment_Phrase::analyze(Environ& env) const
{
    return make<Literal_Segment>(share(*this),
        String::make(location().range()));
}
Shared<Segment>
Char_Escape_Phrase::analyze(Environ& env) const
{
    return make<Literal_Segment>(share(*this),
        String::make(location().range().first+1, 1));
}
Shared<Segment>
Paren_Segment_Phrase::analyze(Environ& env) const
{
    return make<Paren_Segment>(share(*this), analyze_op(*expr_, env));
}
Shared<Segment>
Brace_Segment_Phrase::analyze(Environ& env) const
{
    return make<Brace_Segment>(share(*this), analyze_op(*expr_, env));
}
Shared<Meaning>
String_Phrase_Base::analyze(Environ& env) const
{
    return analyze_string(env);
}
Shared<String_Expr>
String_Phrase_Base::analyze_string(Environ& env) const
{
    std::vector<Shared<Segment>> ops;
    for (Shared<const Segment_Phrase> seg : *this)
        ops.push_back(seg->analyze(env));
    return String_Expr::make_elements(ops, this);
}

Shared<Meaning>
Unary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind_) {
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
        return make<Spread_Op>(
            share(*this),
            analyze_op(*arg_, env));
    case Token::k_var:
        throw Exception(At_Token(op_, *this, env), "syntax error");
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
    Atom_Map<slot_t> params;
    slot_t slot = 0;
    each_argument(*left_, [&](const Phrase& p)->void {
        if (auto id = dynamic_cast<const Identifier*>(&p))
            params[id->atom_] = slot++;
        else
            throw Exception(At_Phrase(p, env), "not a parameter");
    });

    // Phase 2: make an Environ from the parameters and analyze the body.
    struct Arg_Environ : public Environ
    {
        Atom_Map<slot_t>& names_;
        Shared<Module::Dictionary> nonlocal_dictionary_ =
            make<Module::Dictionary>();
        std::vector<Shared<Operation>> nonlocal_exprs_;
        bool shared_nonlocals_;

        Arg_Environ(
            Environ* parent, Atom_Map<slot_t>& names, bool shared_nonlocals)
        :
            Environ(parent), names_(names), shared_nonlocals_(shared_nonlocals)
        {
            frame_nslots_ = names.size();
            frame_maxslots_ = names.size();
        }
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            auto p = names_.find(id.atom_);
            if (p != names_.end())
                return make<Arg_Ref>(share(id), p->second);
            if (shared_nonlocals_)
                return parent_->single_lookup(id);
            auto n = nonlocal_dictionary_->find(id.atom_);
            if (n != nonlocal_dictionary_->end())
                return make<Nonlocal_Strict_Ref>(share(id), n->second);
            auto m = parent_->lookup(id);
            if (isa<Constant>(m))
                return m;
            if (auto expr = cast<Operation>(m)) {
                slot_t slot = nonlocal_exprs_.size();
                (*nonlocal_dictionary_)[id.atom_] = slot;
                nonlocal_exprs_.push_back(expr);
                return make<Nonlocal_Strict_Ref>(share(id), slot);
            }
            return m;
        }
    };
    Arg_Environ env2(&env, params, shared_nonlocals_);
    auto expr = analyze_op(*right_, env2);
    auto src = share(*this);

    auto nonlocals = make<Enum_Module_Expr>(src,
        std::move(env2.nonlocal_dictionary_),
        std::move(env2.nonlocal_exprs_));

    return make<Lambda_Expr>(
        src, expr, nonlocals, params.size(), env2.frame_maxslots_);
}

Shared<Meaning>
analyze_assoc(Environ& env,
    const Phrase& src, const Phrase& left, Shared<Phrase> right)
{
    if (auto id = dynamic_cast<const Identifier*>(&left))
        return make<Assoc>(share(src),
            Atom_Expr{share(*id)}, analyze_op(*right, env));
    if (auto string = dynamic_cast<const String_Phrase*>(&left)) {
        auto string_expr = string->analyze_string(env);
        return make<Assoc>(share(src),
            Atom_Expr{string_expr}, analyze_op(*right, env));
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return analyze_assoc(env, src, *call->function_,
            make<Lambda_Phrase>(call->arg_, Token(), right));
    throw Exception(At_Phrase(left,  env), "invalid definiendum");
}

/// In the grammar, a <list> phrase is zero or more constituent phrases
/// separated by commas or semicolons.
/// This function iterates over each constituent phrase.
static inline void
each_item(Phrase& phrase, std::function<void(Phrase&)> func)
{
    if (dynamic_cast<Empty_Phrase*>(&phrase))
        return;
    if (auto commas = dynamic_cast<Comma_Phrase*>(&phrase)) {
        for (auto& i : commas->args_)
            func(*i.expr_);
        return;
    }
    if (auto semis = dynamic_cast<Semicolon_Phrase*>(&phrase)) {
        for (auto& i : semis->args_)
            func(*i.expr_);
        return;
    }
    func(phrase);
}

Shared<Meaning>
analyze_block(
    Environ& env,
    Shared<const Phrase> source,
    Definition::Kind kind,
    Shared<Phrase> bindings,
    Shared<const Phrase> bodysrc)
{
    Shared<Definition> adef = bindings->as_definition(env);
    if (adef == nullptr) {
        // no definitions, just actions.
        return make<Preaction_Op>(
            source,
            analyze_op(*bindings, env),
            analyze_op(*bodysrc, env));
    }
    if (adef->kind_ == Definition::k_sequential
        && kind == Definition::k_sequential)
    {
        Sequential_Scope sscope(env, false);
        sscope.analyze(*adef);
        sscope.is_analyzing_action_ = env.is_analyzing_action_;
        auto body = analyze_tail(*bodysrc, sscope);
        env.frame_maxslots_ = sscope.frame_maxslots_;
        return make<Block_Op>(source,
            std::move(sscope.executable_), std::move(body));
    }
    if (adef->kind_ == Definition::k_recursive
        && kind == Definition::k_recursive)
    {
        Recursive_Scope rscope(env, false);
        rscope.analyze(*adef);
        auto body = analyze_tail(*bodysrc, rscope);
        env.frame_maxslots_ = rscope.frame_maxslots_;
        return make<Block_Op>(source,
            std::move(rscope.executable_), std::move(body));
    }
    struct Bad_Scope : public Scope
    {
        Environ& env_;

        Bad_Scope(Environ& env) : env_(env) {}

        virtual void analyze(Definition&) override {}
        virtual void add_action(Shared<const Phrase>) override {}
        virtual unsigned begin_unit(Shared<Unitary_Definition> unit) override
        {
            throw Exception(At_Phrase(*unit->source_, env_),
                "wrong style of definition for this block");
        }
        virtual slot_t add_binding(Shared<const Identifier>, unsigned) override
        {
            return 0;
        }
        virtual void end_unit(unsigned, Shared<Unitary_Definition>) override {}
    } bscope(env);
    adef->add_to_scope(bscope); // throws an exception
    assert(0);
}

Shared<Meaning>
Let_Phrase::analyze(Environ& env) const
{
    Definition::Kind kind =
        let_.kind_ == Token::k_let
        ? Definition::k_recursive
        : Definition::k_sequential;
    return analyze_block(env, share(*this), kind, bindings_, body_);
}

Shared<Meaning>
Binary_Phrase::analyze(Environ& env) const
{
    switch (op_.kind_) {
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
        if (auto id = cast<const Identifier>(right_)) {
            return make<Dot_Expr>(
                share(*this),
                analyze_op(*left_, env),
                Atom_Expr{id});
        }
        if (auto string = cast<const String_Phrase>(right_)) {
            auto str_expr = string->analyze_string(env);
            return make<Dot_Expr>(
                share(*this),
                analyze_op(*left_, env),
                Atom_Expr{str_expr});
        }
        throw Exception(At_Phrase(*right_, env),
            "invalid expression after '.'");
    case Token::k_in:
        throw Exception(At_Token(op_, *this, env), "syntax error");
    case Token::k_apostrophe:
        return make<Index_Expr>(
            share(*this),
            analyze_op(*left_, env),
            analyze_op(*right_, env));
    case Token::k_colon:
        return analyze_assoc(env, *this, *left_, right_);
    case Token::k_where:
        return analyze_block(env, share(*this),
            Definition::k_recursive, right_, left_);
    default:
        assert(0);
    }
}

Shared<Meaning>
Recursive_Definition_Phrase::analyze(Environ& env) const
{
    throw Exception(At_Phrase(*this, env), "not an operation");
}
Shared<Meaning>
Sequential_Definition_Phrase::analyze(Environ& env) const
{
    throw Exception(At_Phrase(*this, env), "not an operation");
}
Shared<Meaning>
Assignment_Phrase::analyze(Environ& env) const
{
    auto id = cast<Identifier>(left_);
    if (id == nullptr)
        throw Exception(At_Phrase(*left_, env), "not a variable name");
    auto m = env.lookup_var(*id);
    auto expr = analyze_op(*right_, env);

    auto let = cast<Let_Ref>(m);
    if (let)
        return make<Data_Setter>(share(*this), let->slot_, expr, true);
    auto indir = cast<Indirect_Strict_Ref>(m);
    if (indir)
        return make<Module_Data_Setter>(share(*this),
            indir->slot_, indir->index_, expr);

    // this should never happen
    throw Exception(At_Phrase(*left_, env), "not a sequential variable name");
}

Shared<Definition>
as_definition_iter(
    Environ& env, Shared<const Phrase> source,
    Phrase& left, Shared<Phrase> right, Definition::Kind kind)
{
    if (auto id = dynamic_cast<const Identifier*>(&left)) {
        auto lambda = cast<Lambda_Phrase>(right);
        if (lambda && kind == Definition::k_recursive)
            return make<Function_Definition>(std::move(source),
                share(*id), std::move(lambda));
        else
            return make<Data_Definition>(std::move(source), kind,
                share(*id), std::move(right));
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return as_definition_iter(env, std::move(source), *call->function_,
            make<Lambda_Phrase>(call->arg_, Token(), right), kind);
    throw Exception(At_Phrase(left,  env), "invalid definiendum");
}
Shared<Definition>
Recursive_Definition_Phrase::as_definition(Environ& env)
{
    return as_definition_iter(env, share(*this), *left_, right_,
        Definition::k_recursive);
}
Shared<Definition>
Sequential_Definition_Phrase::as_definition(Environ& env)
{
    return as_definition_iter(env, share(*this), *left_, right_,
        Definition::k_sequential);
}

Shared<Meaning>
Semicolon_Phrase::analyze(Environ& env) const
{
    Shared<Compound_Op> compound = Compound_Op::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        compound->at(i) = analyze_action(*args_[i].expr_, env);
    return compound;
}

Shared<Definition>
Semicolon_Phrase::as_definition(Environ& env)
{
    Shared<Compound_Definition> compound =
        Compound_Definition::make(args_.size(), share(*this));
    bool have_kind = false;
    for (size_t i = 0; i < args_.size(); ++i) {
        auto phrase = args_[i].expr_;
        compound->at(i).phrase_ = phrase;
        auto def = args_[i].expr_->as_definition(env);
        if (def) {
            if (!have_kind) {
                compound->kind_ = def->kind_;
                have_kind = true;
            } else if (compound->kind_ != def->kind_) {
                throw Exception(At_Phrase(*phrase, env),
                "conflicting definition types in the same compound definition");
            }
        }
        compound->at(i).definition_ = def;
    }
    if (have_kind)
        return compound;
    else
        return nullptr;
}

Shared<Meaning>
Comma_Phrase::analyze(Environ& env) const
{
    throw Exception(At_Token(args_[0].separator_, *this, env), "syntax error");
}

Shared<Meaning>
Paren_Phrase::analyze(Environ& env) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyze_op(*items[i].expr_, env);
        return list;
    } else
        return analyze_tail(*body_, env);
}
Shared<Definition>
Paren_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Bracket_Phrase::analyze(Environ& env) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyze_op(*items[i].expr_, env);
        return list;
    } else {
        Shared<List_Expr> list = List_Expr::make(1, share(*this));
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
    return make<Call_Expr>(
        share(src),
        share(*this),
        analyze_op(*src.arg_, env));
}

std::vector<Shared<Operation>>
Call_Phrase::analyze_args(Environ& env) const
{
    std::vector<Shared<Operation>> argv;
    each_argument(*arg_, [&](const Phrase& p)->void {
        argv.push_back(analyze_op(p, env));
    });
    return std::move(argv);
}

Shared<Meaning>
Program_Phrase::analyze(Environ& env) const
{
    return body_->analyze(env);
}
Shared<Definition>
Program_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Brace_Phrase::analyze(Environ& env) const
{
    Shared<Definition> adef = body_->as_definition(env);
    if (adef == nullptr) {
        auto record = make<Record_Expr>(share(*this));
        each_item(*body_, [&](Phrase& item)->void {
            record->fields_.push_back(analyze_op(item, env));
        });
        return record;
    }
    return analyze_module(*adef, env);
}

Shared<Meaning>
If_Phrase::analyze(Environ& env) const
{
    if (else_expr_ == nullptr) {
        return make<If_Op>(
            share(*this),
            analyze_op(*condition_, env),
            analyze_tail(*then_expr_, env));
    } else {
        return make<If_Else_Op>(
            share(*this),
            analyze_op(*condition_, env),
            analyze_tail(*then_expr_, env),
            analyze_tail(*else_expr_, env));
    }
}

Shared<Meaning>
For_Phrase::analyze(Environ& env) const
{
    Atom name = id_->atom_;

    auto list = analyze_op(*listexpr_, env);

    slot_t slot = env.frame_nslots_;
    struct For_Environ : public Environ
    {
        Atom name_;
        slot_t slot_;

        For_Environ(
            Environ& p,
            Atom name,
            slot_t slot)
        : Environ(&p), name_(name), slot_(slot)
        {
            frame_nslots_ = p.frame_nslots_ + 1;
            frame_maxslots_ = std::max(p.frame_maxslots_, frame_nslots_);
            is_analyzing_action_ = p.is_analyzing_action_;
        }
        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            if (id.atom_ == name_)
                return make<Let_Ref>(share(id), slot_);
            return nullptr;
        }
    };
    For_Environ body_env(env, name, slot);
    auto body = analyze_tail(*body_, body_env);
    env.frame_maxslots_ = body_env.frame_maxslots_;

    return make<For_Op>(share(*this), slot, list, body);
}

Shared<Meaning>
While_Phrase::analyze(Environ& env) const
{
    auto cond = analyze_op(*args_, env);
    auto body = analyze_tail(*body_, env);
    return make<While_Action>(share(*this), cond, body);
}

Shared<Meaning>
Range_Phrase::analyze(Environ& env) const
{
    return make<Range_Expr>(
        share(*this),
        analyze_op(*first_, env),
        analyze_op(*last_, env),
        step_ ? analyze_op(*step_, env) : nullptr,
        op1_.kind_ == Token::k_open_range);
}

} // namespace curv
