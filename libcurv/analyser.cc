// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <functional>

#include <libcurv/phrase.h>
#include <libcurv/analyser.h>
#include <libcurv/definition.h>
#include <libcurv/die.h>
#include <libcurv/shared.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/context.h>
#include <libcurv/system.h>

namespace curv
{

Shared<Operation>
analyse_op(const Phrase& ph, Environ& env)
{
    bool old = env.is_analysing_action_;
    env.is_analysing_action_ = false;
    auto result = ph.analyse(env)->to_operation(env.file_frame_);
    env.is_analysing_action_ = old;
    return result;
}

Shared<Operation>
analyse_action(const Phrase& ph, Environ& env)
{
    bool old = env.is_analysing_action_;
    env.is_analysing_action_ = true;
    auto result = ph.analyse(env)->to_operation(env.file_frame_);
    env.is_analysing_action_ = old;
    return result;
}

Shared<Operation>
analyse_tail(const Phrase& ph, Environ& env)
{
    return ph.analyse(env)->to_operation(env.file_frame_);
}

// Evaluate the phrase as a constant expression in the builtin environment.
Value
std_eval(const Phrase& ph, Environ& env)
{
    Builtin_Environ benv(
        env.system_.std_namespace(), env.system_, env.file_frame_);
    auto op = analyse_op(ph, benv);
    auto frame = Frame::make(benv.frame_maxslots_,
        env.system_, env.file_frame_, nullptr, nullptr);
    return op->eval(*frame);
}

Shared<Operation>
Meaning::to_operation(Frame* f)
{
    throw Exception(At_Phrase(*syntax_, f), "not an operation");
}

Shared<Meaning>
Meaning::call(const Call_Phrase&, Environ& env)
{
    throw Exception(At_Phrase(*syntax_, env), "not callable");
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
    throw Exception(At_Phrase(id, *this), stringify(id.symbol_,": not defined"));
}

Shared<Meaning>
Environ::lookup_var(const Identifier& id)
{
    for (Environ* e = this; e != nullptr; e = e->parent_) {
        if (!e->is_analysing_action_)
            break;
        if (e->is_sequential_statement_list_) {
            auto m = e->single_lookup(id);
            if (m != nullptr)
                return m;
        }
    }
    throw Exception(At_Phrase(id, *this),
        stringify("var ",id.symbol_,": not defined"));
}

Shared<Meaning>
Builtin_Environ::single_lookup(const Identifier& id)
{
    auto p = names.find(id.symbol_);
    if (p != names.end())
        return p->second->to_meaning(id);
    return nullptr;
}

Shared<Meaning>
Empty_Phrase::analyse(Environ& env) const
{
    return make<Null_Action>(share(*this));
}

Shared<Meaning>
Identifier::analyse(Environ& env) const
{
    return env.lookup(*this);
}

Shared<Meaning>
Numeral::analyse(Environ& env) const
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
                die("Numeral::analyse: bad hex numeral");
        }
        return make<Constant>(share(*this), n);
      }
    default:
        die("Numeral::analyse: bad token type");
    }
}

Shared<Segment>
String_Segment_Phrase::analyse(Environ& env) const
{
    return make<Literal_Segment>(share(*this),
        String::make(location().range()));
}
Shared<Segment>
Char_Escape_Phrase::analyse(Environ& env) const
{
    return make<Literal_Segment>(share(*this),
        String::make(location().range().first+1, 1));
}
Shared<Segment>
Paren_Segment_Phrase::analyse(Environ& env) const
{
    return make<Paren_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Bracket_Segment_Phrase::analyse(Environ& env) const
{
    return make<Bracket_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Brace_Segment_Phrase::analyse(Environ& env) const
{
    return make<Brace_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Meaning>
String_Phrase_Base::analyse(Environ& env) const
{
    return analyse_string(env);
}
Shared<String_Expr>
String_Phrase_Base::analyse_string(Environ& env) const
{
    std::vector<Shared<Segment>> ops;
    for (Shared<const Segment_Phrase> seg : *this)
        ops.push_back(seg->analyse(env));
    return String_Expr::make_elements(ops, this);
}

Shared<Meaning>
Unary_Phrase::analyse(Environ& env) const
{
    switch (op_.kind_) {
    case Token::k_not:
        return make<Not_Expr>(
            share(*this),
            analyse_op(*arg_, env));
    case Token::k_plus:
        return make<Positive_Expr>(
            share(*this),
            analyse_op(*arg_, env));
    case Token::k_minus:
        return make<Negative_Expr>(
            share(*this),
            analyse_op(*arg_, env));
    case Token::k_ellipsis:
        return make<Spread_Op>(
            share(*this),
            analyse_op(*arg_, env));
    case Token::k_include:
    case Token::k_var:
        throw Exception(At_Token(op_, *this, env), "syntax error");
    default:
        die("Unary_Phrase::analyse: bad operator token type");
    }
}
Shared<Definition>
Unary_Phrase::as_definition(Environ& env)
{
    switch (op_.kind_) {
    case Token::k_include:
        return make<Include_Definition>(share(*this), arg_);
    default:
        return nullptr;
    }
}

Shared<Meaning>
Lambda_Phrase::analyse(Environ& env) const
{
    struct Arg_Scope : public Scope
    {
        bool shared_nonlocals_;
        Shared<Module::Dictionary> nonlocal_dictionary_ =
            make<Module::Dictionary>();
        std::vector<Shared<Operation>> nonlocal_exprs_;

        Arg_Scope(Environ& parent, bool shared_nonlocals)
        :
            Scope(parent),
            shared_nonlocals_(shared_nonlocals)
        {
            frame_nslots_ = 0;
            frame_maxslots_ = 0;
        }

        virtual Shared<Meaning> single_lookup(const Identifier& id)
        {
            auto b = dictionary_.find(id.symbol_);
            if (b != dictionary_.end())
                return make<Data_Ref>(share(id), b->second.slot_index_);
            if (shared_nonlocals_)
                return parent_->single_lookup(id);
            auto n = nonlocal_dictionary_->find(id.symbol_);
            if (n != nonlocal_dictionary_->end())
                return make<Nonlocal_Data_Ref>(share(id), n->second);
            auto m = parent_->lookup(id);
            if (isa<Constant>(m))
                return m;
            if (auto expr = cast<Operation>(m)) {
                slot_t slot = nonlocal_exprs_.size();
                (*nonlocal_dictionary_)[id.symbol_] = slot;
                nonlocal_exprs_.push_back(expr);
                return make<Nonlocal_Data_Ref>(share(id), slot);
            }
            return m;
        }
    } scope(env, shared_nonlocals_);

    auto src = share(*this);
    auto pattern = make_pattern(*left_, scope, 0);
    pattern->analyse(scope);
    auto expr = analyse_op(*right_, scope);
    auto nonlocals = make<Enum_Module_Expr>(src,
        std::move(scope.nonlocal_dictionary_),
        std::move(scope.nonlocal_exprs_));

    return make<Lambda_Expr>(
        src, pattern, expr, nonlocals, scope.frame_maxslots_);
}

Shared<Meaning>
analyse_assoc(Environ& env,
    const Phrase& src, const Phrase& left, Shared<Phrase> right)
{
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return analyse_assoc(env, src, *call->function_,
            make<Lambda_Phrase>(call->arg_, Token(), right));

    Shared<Operation> right_expr;
    if (isa<Empty_Phrase>(right))
        right_expr = make<Constant>(right, Value{true});
    else
        right_expr = analyse_op(*right, env);

    if (auto id = dynamic_cast<const Identifier*>(&left))
        return make<Assoc>(share(src), Symbol_Expr{share(*id)}, right_expr);
    if (auto string = dynamic_cast<const String_Phrase*>(&left)) {
        auto string_expr = string->analyse_string(env);
        return make<Assoc>(share(src), Symbol_Expr{string_expr}, right_expr);
    }

    throw Exception(At_Phrase(left,  env), "invalid definiendum");
}

/// In the grammar, a <list> phrase is zero or more constituent phrases
/// separated by commas or semicolons.
/// This function iterates over each constituent phrase.
void
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
analyse_block(
    Environ& env,
    Shared<const Phrase> syntax,
    Definition::Kind kind,
    Shared<Phrase> bindings,
    Shared<const Phrase> bodysrc)
{
    Shared<Definition> adef = bindings->as_definition(env);
    if (adef == nullptr) {
        // no definitions, just actions.
        return make<Preaction_Op>(
            syntax,
            analyse_op(*bindings, env),
            analyse_tail(*bodysrc, env));
    }
    if (adef->kind_ == Definition::k_sequential
        && kind == Definition::k_sequential)
    {
        Sequential_Scope sscope(env, false);
        sscope.analyse(*adef);
        sscope.is_analysing_action_ = env.is_analysing_action_;
        auto body = analyse_tail(*bodysrc, sscope);
        env.frame_maxslots_ = sscope.frame_maxslots_;
        return make<Block_Op>(syntax,
            std::move(sscope.executable_), std::move(body));
    }
    if (adef->kind_ == Definition::k_recursive
        && kind == Definition::k_recursive)
    {
        Recursive_Scope rscope(env, false, adef->syntax_);
        rscope.analyse(*adef);
        rscope.is_analysing_action_ = env.is_analysing_action_;
        auto body = analyse_tail(*bodysrc, rscope);
        env.frame_maxslots_ = rscope.frame_maxslots_;
        return make<Block_Op>(syntax,
            std::move(rscope.executable_), std::move(body));
    }
    struct Bad_Scope : public Block_Scope
    {
        Bad_Scope(Environ& env) : Block_Scope(env, false) {}

        virtual Shared<Meaning> single_lookup(const Identifier&) override
        {
            return nullptr;
        }
        virtual void analyse(Definition&) override {}
        virtual void add_action(Shared<const Phrase>) override {}
        virtual unsigned begin_unit(Shared<Unitary_Definition> unit) override
        {
            throw Exception(At_Phrase(*unit->syntax_, *parent_),
                "wrong style of definition for this block");
        }
        virtual slot_t add_binding(Symbol, const Phrase&, unsigned) override
        {
            return 0;
        }
        virtual void end_unit(unsigned, Shared<Unitary_Definition>) override {}
    } bscope(env);
    adef->add_to_scope(bscope); // throws an exception
    die("analyse_block: add_to_scope failed to throw an exception");
}

Shared<Meaning>
Let_Phrase::analyse(Environ& env) const
{
    Definition::Kind kind =
        let_.kind_ == Token::k_let
        ? Definition::k_recursive
        : Definition::k_sequential;
    return analyse_block(env, share(*this), kind, bindings_, body_);
}

Shared<Meaning>
Where_Phrase::analyse(Environ& env) const
{
    // Deprecation warning if right argument not parenthesized.
    if (!isa<const Paren_Phrase>(right_)) {
        Exception exc{At_Token(right_->location().starting_at(op_), env),
            "right argument of `where` must be parenthesized"};
        env.system_.message("DEPRECATION WARNING: ", exc);
    }

    Shared<const Phrase> syntax = share(*this);
    Shared<Phrase> bindings = right_;
    Shared<const Phrase> bodysrc = left_;
    auto let = cast<const Let_Phrase>(bodysrc);
    if (let && let->let_.kind_ == Token::k_let)
    {
        // let bindings1 in body where bindings2
        Shared<Definition> adef1 = let->bindings_->as_definition(env);
        Shared<Definition> adef2 = bindings->as_definition(env);
        if (adef1 && adef1->kind_ == Definition::k_recursive
            && adef2 && adef2->kind_ == Definition::k_recursive)
        {
            Recursive_Scope rscope(env, false, syntax);
            adef1->add_to_scope(rscope);
            adef2->add_to_scope(rscope);
            rscope.analyse();
            rscope.is_analysing_action_ = env.is_analysing_action_;
            auto body = analyse_tail(*let->body_, rscope);
            env.frame_maxslots_ = rscope.frame_maxslots_;
            return make<Block_Op>(syntax,
                std::move(rscope.executable_), std::move(body));
        }
    }
    return analyse_block(env, syntax,
        Definition::k_recursive, bindings, bodysrc);
}

Shared<Meaning>
Binary_Phrase::analyse(Environ& env) const
{
    switch (op_.kind_) {
    case Token::k_or:
        return make<Or_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_and:
        return make<And_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_equal:
        return make<Equal_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_not_equal:
        return make<Not_Equal_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_less:
        return make<Less_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_greater:
        return make<Greater_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_less_or_equal:
        return make<Less_Or_Equal_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_greater_or_equal:
        return make<Greater_Or_Equal_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_plus:
        return make<Add_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_minus:
        return make<Subtract_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_times:
        return make<Multiply_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_over:
        return make<Divide_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_power:
        return make<Power_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_dot:
        if (auto id = cast<const Identifier>(right_)) {
            return make<Dot_Expr>(
                share(*this),
                analyse_op(*left_, env),
                Symbol_Expr{id});
        }
        if (auto string = cast<const String_Phrase>(right_)) {
            auto str_expr = string->analyse_string(env);
            return make<Dot_Expr>(
                share(*this),
                analyse_op(*left_, env),
                Symbol_Expr{str_expr});
        }
        throw Exception(At_Phrase(*right_, env),
            "invalid expression after '.'");
    case Token::k_in:
        throw Exception(At_Token(op_, *this, env), "syntax error");
    case Token::k_apostrophe:
        return make<Index_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    case Token::k_colon:
        return analyse_assoc(env, *this, *left_, right_);
    default:
        die("Binary_Phrase::analyse: bad operator token type");
    }
}

Shared<Meaning>
Recursive_Definition_Phrase::analyse(Environ& env) const
{
    throw Exception(At_Phrase(*this, env), "not an operation");
}
Shared<Meaning>
Sequential_Definition_Phrase::analyse(Environ& env) const
{
    throw Exception(At_Phrase(*this, env), "not an operation");
}
Shared<Meaning>
Assignment_Phrase::analyse(Environ& env) const
{
    auto id = cast<Identifier>(left_);
    if (id == nullptr)
        throw Exception(At_Phrase(*left_, env), "not a variable name");
    auto m = env.lookup_var(*id);
    auto expr = analyse_op(*right_, env);

    auto let = cast<Data_Ref>(m);
    if (let)
        return make<Data_Setter>(share(*this), let->slot_, expr, true);
    auto indir = cast<Module_Data_Ref>(m);
    if (indir)
        return make<Module_Data_Setter>(share(*this),
            indir->slot_, indir->index_, expr);

    // this should never happen
    throw Exception(At_Phrase(*left_, env), "not a sequential variable name");
}

Shared<Definition>
as_definition_iter(
    Environ& env, Shared<const Phrase> syntax,
    Phrase& left, Shared<Phrase> right, Definition::Kind kind)
{
    if (auto id = dynamic_cast<const Identifier*>(&left)) {
        auto lambda = cast<Lambda_Phrase>(right);
        if (lambda && kind == Definition::k_recursive)
            return make<Function_Definition>(std::move(syntax),
                share(*id), std::move(lambda));
        else
            return make<Data_Definition>(std::move(syntax), kind,
                share(*id), std::move(right));
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return as_definition_iter(env, std::move(syntax), *call->function_,
            make<Lambda_Phrase>(call->arg_, Token(), right), kind);
    return make<Data_Definition>(std::move(syntax), kind,
        share(left), std::move(right));
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
Semicolon_Phrase::analyse(Environ& env) const
{
    Shared<Compound_Op> compound = Compound_Op::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        compound->at(i) = analyse_action(*args_[i].expr_, env);
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
Comma_Phrase::analyse(Environ& env) const
{
    throw Exception(At_Token(args_[0].separator_, *this, env), "syntax error");
}

Shared<Meaning>
Paren_Phrase::analyse(Environ& env) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env);
        return list;
    } else {
        // One of the few places we directly call Phrase::analyse().
        // The result can be an operation or a metafunction.
        return body_->analyse(env);
    }
}
Shared<Definition>
Paren_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Bracket_Phrase::analyse(Environ& env) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env);
        return list;
    } else {
        Shared<List_Expr> list = List_Expr::make(1, share(*this));
        (*list)[0] = analyse_op(*body_, env);
        return list;
    }
}

Shared<Meaning>
Call_Phrase::analyse(Environ& env) const
{
    return function_->analyse(env)->call(*this, env);
}

Shared<Meaning>
Operation::call(const Call_Phrase& src, Environ& env)
{
    return make<Call_Expr>(
        share(src),
        share(*this),
        analyse_op(*src.arg_, env));
}

Shared<Meaning>
Program_Phrase::analyse(Environ& env) const
{
    return body_->analyse(env);
}
Shared<Definition>
Program_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Brace_Phrase::analyse(Environ& env) const
{
    Shared<Definition> adef = body_->as_definition(env);
    if (adef == nullptr) {
        auto record = make<Record_Expr>(share(*this));
        each_item(*body_, [&](Phrase& item)->void {
            record->fields_.push_back(analyse_op(item, env));
        });
        return record;
    }
    return analyse_module(*adef, env);
}

Shared<Meaning>
If_Phrase::analyse(Environ& env) const
{
    if (else_expr_ == nullptr) {
        return make<If_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_tail(*then_expr_, env));
    } else {
        return make<If_Else_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_tail(*then_expr_, env),
            analyse_tail(*else_expr_, env));
    }
}

Shared<Meaning>
For_Phrase::analyse(Environ& env) const
{
    Scope scope(env);
    scope.is_analysing_action_ = env.is_analysing_action_;

    auto pat = make_pattern(*pattern_, scope, 0);
    pat->analyse(scope);
    auto list = analyse_op(*listexpr_, env);
    auto body = analyse_tail(*body_, scope);

    env.frame_maxslots_ = scope.frame_maxslots_;
    return make<For_Op>(share(*this), pat, list, body);
}

Shared<Meaning>
While_Phrase::analyse(Environ& env) const
{
    auto cond = analyse_op(*args_, env);
    auto body = analyse_tail(*body_, env);
    return make<While_Action>(share(*this), cond, body);
}

Shared<Meaning>
Range_Phrase::analyse(Environ& env) const
{
    return make<Range_Expr>(
        share(*this),
        analyse_op(*first_, env),
        analyse_op(*last_, env),
        step_ ? analyse_op(*step_, env) : nullptr,
        op1_.kind_ == Token::k_open_range);
}

} // namespace curv
