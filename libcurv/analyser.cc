// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>

#include <libcurv/context.h>
#include <libcurv/definition.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/phrase.h>
#include <libcurv/shared.h>
#include <libcurv/symbol.h>
#include <libcurv/system.h>

#include <functional>

namespace curv
{

Shared<Operation>
analyse_op(const Phrase& ph, Environ& env, unsigned edepth)
{
    return ph.analyse(env, edepth)
        ->to_operation(env.analyser_.system_, env.analyser_.file_frame_);
}

// Evaluate the phrase as a constant expression in the builtin environment.
Value
std_eval(const Phrase& ph, Environ& env)
{
    Builtin_Environ benv(
        env.analyser_.system_.std_namespace(), env.analyser_);
    auto op = analyse_op(ph, benv);
    auto frame = Frame::make(benv.frame_maxslots_,
        env.analyser_.system_, env.analyser_.file_frame_, nullptr, nullptr);
    return op->eval(*frame);
}

Shared<Operation>
Meaning::to_operation(System& sys, Frame* f)
{
    throw Exception(At_Phrase(*syntax_, sys, f), "not an expression or statement");
}

Shared<Meaning>
Meaning::call(const Call_Phrase&, Environ& env)
{
    throw Exception(At_Phrase(*syntax_, env), "not callable");
}

Shared<Operation>
Operation::to_operation(System&, Frame*)
{
    return share(*this);
}

Shared<Definition>
Phrase::as_definition(Environ&)
{
    return nullptr;
}

// Scope object for analysing a Lambda body. Contains both parameters and
// nonlocals. We don't classify function parameters as 'local variables' that
// are assignable using := for several reasons:
//  1. The 'edepth' mechanism won't let us classify parameters as assignable
//     local variables, while keeping out nonlocals, because both are mixed
//     together in the same scope object.
//  2. In a curried function, all parameters except the last are actually
//     implemented as nonlocals. If we copied nonlocal parameters into data
//     slots then they could be assignable.
//  3. In GLSL, formal parameters are nonassignable unless they are 'out'
//     parameters. Again, this is fixable by copying the parameter value
//     into a local variable in the SC compiler.
struct Lambda_Scope : public Scope
{
    bool shared_nonlocals_;
    Shared<Module::Dictionary> nonlocal_dictionary_ =
        make<Module::Dictionary>();
    std::vector<Shared<Operation>> nonlocal_exprs_;

    Lambda_Scope(Environ& parent, bool shared_nonlocals)
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
};

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

Shared<Locative>
Environ::single_lvar_lookup(const Identifier& id)
{
    auto m = single_lookup(id);
    if (m != nullptr) {
        auto dref = cast<Data_Ref>(m);
        auto lambda = dynamic_cast<Lambda_Scope*>(this);
        if (dref == nullptr || lambda != nullptr) {
            throw Exception(At_Phrase(id, *this),
                stringify(id.symbol_,": not assignable"));
        }
        return make<Local_Locative>(share(id), dref->slot_);
    }
    return nullptr;
}

Shared<Locative>
Environ::lookup_lvar(const Identifier& id, unsigned edepth)
{
    Environ *env = this;
    for (; env != nullptr; env = env->parent_) {
        if (edepth == 0)
            break;
        --edepth;
        auto loc = env->single_lvar_lookup(id);
        if (loc != nullptr)
            return loc;
    }
    // Figure out what went wrong and give a good error.
    for (; env != nullptr; env = env->parent_) {
        auto loc = env->single_lvar_lookup(id);
        if (loc != nullptr) {
            throw Exception(At_Phrase(id, *this), stringify(
                id.symbol_,": not assignable from inside an expression"));
        }
    }
    throw Exception(At_Phrase(id, *this),
        stringify(id.symbol_,": not defined"));
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
Empty_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Null_Action>(share(*this));
}

Shared<Meaning>
Identifier::analyse(Environ& env, unsigned) const
{
    return env.lookup(*this);
}

Value
Numeral::eval() const
{
    switch (loc_.token().kind_) {
    case Token::k_num:
      {
        std::string str(location().range());
        char* endptr;
        double n = strtod(str.c_str(), &endptr);
        assert(endptr == str.c_str() + str.size());
        return {n};
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
        return {n};
      }
    case Token::k_symbol:
      {
        auto r = location().range();
        ++r.first;
        auto s = token_to_symbol(r);
        return s.to_value();
      }
    default:
        die("Numeral::analyse: bad token type");
    }
}

Shared<Meaning>
Numeral::analyse(Environ& env, unsigned) const
{
    return make<Constant>(share(*this), eval());
}

Shared<Segment>
String_Segment_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Literal_Segment>(share(*this),
        make_string(location().range()));
}
Shared<Segment>
Char_Escape_Phrase::analyse(Environ& env, unsigned) const
{
    char c;
    if (location().token().kind_ == Token::k_string_newline)
        c = '\n';
    else {
        switch (location().range()[1]) {
        case '=':
            c = '"';
            break;
        case '.':
            c = '$';
            break;
        default:
            die("bad char escape");
        }
    }
    return make<Literal_Segment>(share(*this), make_string(&c, 1));
}
Shared<Segment>
Ident_Segment_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Ident_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Paren_Segment_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Paren_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Bracket_Segment_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Bracket_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Brace_Segment_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Brace_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Meaning>
String_Phrase_Base::analyse(Environ& env, unsigned) const
{
    return analyse_string(env);
}
Shared<String_Expr>
String_Phrase_Base::analyse_string(Environ& env) const
{
    std::vector<Shared<Segment>> ops;
    for (Shared<const Segment_Phrase> seg : *this)
        ops.push_back(seg->analyse(env, 0));
    return String_Expr::make_elements(ops, this);
}

Shared<Meaning>
Unary_Phrase::analyse(Environ& env, unsigned) const
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
    case Token::k_local:
        throw Exception(At_Phrase(*this, env),
            "a local definition must be followed by '; <statement>'");
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

Shared<Lambda_Expr>
analyse_lambda(
    Environ& env,
    Shared<const Phrase> src,
    bool shared_nonlocals,
    Shared<const Phrase> left,
    Shared<const Phrase> right)
{
    Lambda_Scope scope(env, shared_nonlocals);

    auto pattern = make_pattern(*left, false, scope, 0);
    pattern->analyse(scope);
    auto expr = analyse_op(*right, scope, 0);
    auto nonlocals = make<Enum_Module_Expr>(src,
        std::move(scope.nonlocal_dictionary_),
        std::move(scope.nonlocal_exprs_));

    return make<Lambda_Expr>(
        src, pattern, expr, nonlocals, scope.frame_maxslots_);
}

Shared<Meaning>
Lambda_Phrase::analyse(Environ& env, unsigned) const
{
    return analyse_lambda(env, share(*this), shared_nonlocals_, left_, right_);
}

Shared<Meaning>
analyse_assoc(Environ& env,
    const Phrase& src, const Phrase& left, Shared<Phrase> right)
{
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        if (call->op_.kind_ == Token::k_missing)
        {
            return analyse_assoc(env, src, *call->function_,
                make<Lambda_Phrase>(call->arg_, Token(), right));
        }

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

    throw Exception(At_Phrase(left, env), "invalid definiendum");
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
    Shared<const Phrase> bodysrc,
    unsigned edepth)
{
    Shared<Definition> adef = bindings->as_definition(env);
    if (adef == nullptr) {
        // no definitions, just actions.
        return make<Preaction_Op>(
            syntax,
            analyse_op(*bindings, env, edepth),
            analyse_op(*bodysrc, env, edepth));
    }
    if (adef->kind_ == Definition::k_sequential
        && kind == Definition::k_sequential)
    {
        Sequential_Scope sscope(env, false, edepth);
        sscope.analyse(*adef);
        auto body = analyse_op(*bodysrc, sscope, edepth+1);
        env.frame_maxslots_ = sscope.frame_maxslots_;
        return make<Block_Op>(syntax,
            std::move(sscope.executable_), std::move(body));
    }
    if (adef->kind_ == Definition::k_recursive
        && kind == Definition::k_recursive)
    {
        Recursive_Scope rscope(env, false, adef->syntax_);
        rscope.analyse(*adef);
        auto body = analyse_op(*bodysrc, rscope, edepth+1);
        env.frame_maxslots_ = rscope.frame_maxslots_;
        return make<Block_Op>(syntax,
            std::move(rscope.executable_), std::move(body));
    }
    bad_definition(*adef, env, "wrong style of definition for this block");
}

Shared<Meaning>
Let_Phrase::analyse(Environ& env, unsigned edepth) const
{
    if (let_.kind_ == Token::k_parametric) {
        auto ctor = analyse_lambda(env, share(*this), false,
            make<Brace_Phrase>(let_, bindings_, in_),
            body_);
        return make<Parametric_Expr>(share(*this), ctor);
    }
    Definition::Kind kind =
        let_.kind_ == Token::k_let
        ? Definition::k_recursive
        : Definition::k_sequential;
    return analyse_block(env, share(*this), kind, bindings_, body_, edepth);
}

Shared<Meaning>
Where_Phrase::analyse(Environ& env, unsigned edepth) const
{
    Shared<const Phrase> syntax = share(*this);
    Shared<Phrase> bindings = right_;
    Shared<const Phrase> bodysrc = left_;

#if 0 // this logic is in the parser
    // parametric {params} body where bindings
    // is reparsed as
    // parametric {params} (body where bindings)
    // Rationale:
    // 1. 'parametric {params}' can be prefixed to any <list> phrase,
    //    without adding parentheses, and it will work.
    // 2. Recursion between the params and the where bindings is not allowed,
    //    so the params and the bindings cannot share a scope.
    // 3. The params must be visible in the where bindings. There's not a
    //    compelling use case for where bindings to be visible in the params,
    //    and it has to be either one or the other (see #2).
    auto para = cast<const Parametric_Phrase>(bodysrc);
    if (para) {
        auto newparse = make<Parametric_Phrase>(
            para->keyword_,
            para->param_,
            make<Where_Phrase>(
                para->body_,
                op_,
                bindings));
        return newparse->analyse(env);
    }

    // a = b where bindings
    // is reparsed as
    // a = (b where bindings)
    // Rational: a <where> clause can be affixed to any <list> phrase,
    // without adding parentheses, and it will work.
    //
    // TODO: I would like to permit a definition to be used as the body of
    // a `where` or `let`, so that subexpressions in the left side of the
    // definition are within scope. That's a lot of work to implement. Once
    // done, this code will go away.
    auto def = cast<const Recursive_Definition_Phrase>(bodysrc);
    if (def) {
        auto newparse = make<Recursive_Definition_Phrase>(
            def->left_,
            def->op_,
            make<Where_Phrase>(
                def->right_,
                op_,
                bindings));
        return newparse->analyse(env);
    }
#endif

    // Given 'let bindings1 in body where bindings2',
    // body is analysed in a scope that combines bindings1 and bindings2.
    auto let = cast<const Let_Phrase>(bodysrc);
    if (let && let->let_.kind_ == Token::k_let)
    {
        Shared<Definition> adef1 = let->bindings_->as_definition(env);
        Shared<Definition> adef2 = bindings->as_definition(env);
        if (adef1 && adef1->kind_ == Definition::k_recursive
            && adef2 && adef2->kind_ == Definition::k_recursive)
        {
            Recursive_Scope rscope(env, false, syntax);
            adef1->add_to_scope(rscope);
            adef2->add_to_scope(rscope);
            rscope.analyse();
            auto body = analyse_op(*let->body_, rscope, edepth+1);
            env.frame_maxslots_ = rscope.frame_maxslots_;
            return make<Block_Op>(syntax,
                std::move(rscope.executable_), std::move(body));
        }
    }
    return analyse_block(env, syntax,
        Definition::k_recursive, bindings, bodysrc, edepth);
}

Shared<Meaning>
Binary_Phrase::analyse(Environ& env, unsigned) const
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
    case Token::k_in:
        throw Exception(At_Token(op_, *this, env), "syntax error");
    case Token::k_colon:
        return analyse_assoc(env, *this, *left_, right_);
    default:
        throw Exception(At_Token(op_, *this, env),
            "compiler internal error: "
            "Binary_Phrase::analyse: bad operator token type");
    }
}

Shared<Meaning>
Dot_Phrase::analyse(Environ& env, unsigned) const
{
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
}

// A recursive definition is not an operation.
// But it's easier to report the error at runtime than at analysis time:
// we have more context (we know if we are being evaluated as an expression
// or executed as a statement) so we can give a better error message.
Shared<Meaning>
Recursive_Definition_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Recursive_Definition_Op>(share(*this));
}
Value
Recursive_Definition_Op::eval(Frame& f) const
{
    throw Exception(At_Phrase(*syntax_, f),
        "Not an expression.\n"
        "Maybe try an equality expression (==) instead of a definition (=).");
}
void
Recursive_Definition_Op::exec(Frame& f, Executor&) const
{
    throw Exception(At_Phrase(*syntax_, f),
        "Not a statement.\n"
        "Maybe try an assignment statement (:=) instead of a definition (=).");
}

Shared<Meaning>
Sequential_Definition_Phrase::analyse(Environ& env, unsigned) const
{
    throw Exception(At_Phrase(*this, env), "not an expression or statement");
}

Shared<Locative>
analyse_locative(const Phrase& ph, Environ& env, unsigned edepth)
{
    auto id = dynamic_cast<const Identifier*>(&ph);
    if (id != nullptr)
        return env.lookup_lvar(*id, edepth);
    auto dot = dynamic_cast<const Dot_Phrase*>(&ph);
    if (dot != nullptr) {
        auto base = analyse_locative(*dot->left_, env, edepth);
        // TODO: copypasta from Dot_Phrase::analyse
        // But, edepth is handled differently.
        if (auto id = cast<const Identifier>(dot->right_)) {
            return base->get_field(
                env,
                share(ph),
                Symbol_Expr{id});
        }
        if (auto string = cast<const String_Phrase>(dot->right_)) {
            auto str_expr = string->analyse_string(env);
            return base->get_field(
                env,
                share(ph),
                Symbol_Expr{str_expr});
        }
        throw Exception(At_Phrase(*dot->right_, env),
            "invalid expression after '.'");
    }
    throw Exception(At_Phrase(ph, env), "not a locative");
}

Shared<Meaning>
Assignment_Phrase::analyse(Environ& env, unsigned edepth) const
{
    auto lvar = analyse_locative(*left_, env, edepth);
    auto expr = analyse_op(*right_, env);
    return make<Assignment_Action>(share(*this), lvar, expr);
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
        if (call->op_.kind_ == Token::k_missing)
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
    if (!env.analyser_.var_deprecated_) {
        env.analyser_.system_.warning(Exception{At_Phrase(*this, env),
            "`var` definitions are deprecated"});
        env.analyser_.var_deprecated_ = true;
    }
    return as_definition_iter(env, share(*this), *left_, right_,
        Definition::k_sequential);
}

// Analyse one item in a compound statement, which could be either a
// statement or a local definition.
Shared<Operation>
analyse_stmt(Shared<const Phrase> stmt, Scope& scope, unsigned edepth)
{
    auto unary = cast<const Unary_Phrase>(stmt);
    if (unary != nullptr) {
        if (unary->op_.kind_ == Token::k_local) {
            // Local definitions are part of the syntax of compound statements:
            // they don't have a standalone meaning, and that's why
            // the analysis of local definitions is inline coded here.
            // TODO: support 'local include expr'.
            auto defn = cast<const Recursive_Definition_Phrase>(unary->arg_);
            if (defn == nullptr) {
                throw Exception(At_Phrase(*stmt, scope),
                    "syntax error in local definition (missing =)");
            }
            auto expr = analyse_op(*defn->right_, scope, edepth);
            auto pat = make_pattern(*defn->left_, false, scope, 0);
            pat->analyse(scope);
            return make<Data_Setter>(stmt, slot_t(-1), pat, expr);
        }
    }
    return analyse_op(*stmt, scope, edepth);
}

Shared<Meaning>
Semicolon_Phrase::analyse(Environ& env, unsigned edepth) const
{
    Scope scope(env);
    Shared<Compound_Op> compound = Compound_Op::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        compound->at(i) = analyse_stmt(args_[i].expr_, scope, edepth+1);
    env.frame_maxslots_ = scope.frame_maxslots_;
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
Comma_Phrase::analyse(Environ& env, unsigned) const
{
    throw Exception(At_Token(args_[0].separator_, *this, env), "syntax error");
}

void
List_Expr_Base::init()
{
    pure_ = true;
    for (size_t i = 0; i < size(); ++i) {
        if (!array_[i]->pure_) {
            pure_ = false;
            return;
        }
    }
}

Shared<Meaning>
Paren_Phrase::analyse(Environ& env, unsigned edepth) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env);
        list->init();
        return list;
    } else {
        // One of the few places we directly call Phrase::analyse().
        // The result can be an operation or a metafunction.
        return body_->analyse(env, edepth);
    }
}
Shared<Definition>
Paren_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Bracket_Phrase::analyse(Environ& env, unsigned) const
{
    if (cast<const Empty_Phrase>(body_))
        return List_Expr::make(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env);
        list->init();
        return list;
    } else {
        Shared<List_Expr> list = List_Expr::make(1, share(*this));
        (*list)[0] = analyse_op(*body_, env);
        list->init();
        return list;
    }
}

Shared<Meaning>
Call_Phrase::analyse(Environ& env, unsigned) const
{
    return function_->analyse(env, 0)->call(*this, env);
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
Program_Phrase::analyse(Environ& env, unsigned edepth) const
{
    return body_->analyse(env, edepth);
}
Shared<Definition>
Program_Phrase::as_definition(Environ& env)
{
    return body_->as_definition(env);
}

Shared<Meaning>
Brace_Phrase::analyse(Environ& env, unsigned) const
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
If_Phrase::analyse(Environ& env, unsigned edepth) const
{
    if (else_expr_ == nullptr) {
        return make<If_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_op(*then_expr_, env, edepth));
    } else {
        return make<If_Else_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_op(*then_expr_, env, edepth),
            analyse_op(*else_expr_, env, edepth));
    }
}

Shared<Meaning>
For_Phrase::analyse(Environ& env, unsigned edepth) const
{
    Scope scope(env);

    auto pat = make_pattern(*pattern_, false, scope, 0);
    pat->analyse(scope);
    auto list = analyse_op(*listexpr_, env);
    auto body = analyse_op(*body_, scope, edepth+1);

    env.frame_maxslots_ = scope.frame_maxslots_;
    return make<For_Op>(share(*this), pat, list, body);
}

Shared<Meaning>
While_Phrase::analyse(Environ& env, unsigned edepth) const
{
    auto cond = analyse_op(*args_, env);
    auto body = analyse_op(*body_, env, edepth);
    return make<While_Op>(share(*this), cond, body);
}

Shared<Meaning>
Parametric_Phrase::analyse(Environ& env, unsigned) const
{
    auto ctor = analyse_lambda(env, share(*this), false, param_, body_);
    return make<Parametric_Expr>(share(*this), ctor);
}

Shared<Meaning>
Range_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Range_Expr>(
        share(*this),
        analyse_op(*first_, env),
        analyse_op(*last_, env),
        step_ ? analyse_op(*step_, env) : nullptr,
        op1_.kind_ == Token::k_open_range);
}

Shared<Meaning>
Predicate_Assertion_Phrase::analyse(Environ& env, unsigned) const
{
    return make<Predicate_Assertion_Expr>(
        share(*this),
        analyse_op(*arg_, env),
        analyse_op(*function_, env));
}

} // namespace curv
