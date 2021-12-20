// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>

#include <libcurv/context.h>
#include <libcurv/definition.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/frame.h>
#include <libcurv/function.h>
#include <libcurv/phrase.h>
#include <libcurv/prim_expr.h>
#include <libcurv/shared.h>
#include <libcurv/symbol.h>
#include <libcurv/system.h>
#include <libcurv/tree.h>

#include <functional>

namespace curv
{

void Source_State::deprecate(bool Source_State::* flag, int lvl,
    const Context& cx, String_Ref msg)
{
    if (system_.depr_ >= lvl && (system_.verbose_ || !((*this).*flag))) {
        system_.warning(Exception{cx, msg});
        (*this).*flag = true;
    }
}

const char Source_State::dot_string_deprecated_msg[] =
    "record.\"name\" is deprecated.\n"
    "Use record.'name', record.[#name] instead.";

Shared<Operation>
analyse_op(const Phrase& ph, Environ& env, Interp terp)
{
    return ph.analyse(env, terp)->to_operation(env.sstate_);
}

// Evaluate the phrase as a constant expression in the builtin environment.
Value
std_eval(const Phrase& ph, Environ& env)
{
    Builtin_Environ benv(
        env.sstate_.system_.std_namespace(), env.sstate_);
    auto op = analyse_op(ph, benv);
    auto frame = make_tail_array<Frame>(benv.frame_maxslots_,
        env.sstate_, env.sstate_.file_frame_, nullptr, nullptr);
    return op->eval(*frame);
}

Shared<Operation>
Metafunction::to_operation(Source_State& sstate)
{
    throw Exception(At_Phrase(*syntax_, sstate),
        "This operation must be called with an argument");
}

Shared<Meaning>
Meaning::call(const Call_Phrase&, Environ& env)
{
    throw Exception(At_Phrase(*syntax_, env), "not callable");
}

Shared<Operation>
Operation::to_operation(Source_State&)
{
    return share(*this);
}

Shared<Definition>
Phrase::as_definition(Environ& env, Fail fl) const
{
    FAIL(fl, nullptr, At_Phrase(*this, env), "Not a definition.");
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

    virtual Shared<Meaning> single_lookup(const Identifier& id) override
    {
        auto b = dictionary_.find(id.symbol_);
        if (b != dictionary_.end())
            return make<Local_Data_Ref>(share(id), b->second.slot_index_,
                b->second.variable_);
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
    virtual Unique<const Locative> single_lvar_lookup(const Identifier& id) override
    {
        return Environ::single_lvar_lookup(id);
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

Unique<const Locative>
Environ::single_lvar_lookup(const Identifier& id)
{
    auto m = single_lookup(id);
    if (m != nullptr) {
        throw Exception(At_Phrase(id, *this),
            stringify(id.symbol_,": not assignable"));
    }
    return nullptr;
}

Unique<const Locative>
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
Empty_Phrase::analyse(Environ& env, Interp) const
{
    return make<Null_Action>(share(*this));
}
Shared<Definition>
Empty_Phrase::as_definition(Environ& env, Fail) const
{
    return Compound_Definition::make(0, share(*this));
}

Shared<Meaning>
Identifier::analyse(Environ& env, Interp) const
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
    case Token::k_char:
      {
        auto r = location().range();
        return {r[2]};
      }
    default:
        die("Numeral::analyse: bad token type");
    }
}

Shared<Meaning>
Numeral::analyse(Environ& env, Interp) const
{
    return make<Constant>(share(*this), eval());
}

Shared<Segment>
String_Segment_Phrase::analyse(Environ& env, Interp) const
{
    return make<Literal_Segment>(share(*this),
        make_string(location().range()));
}
Shared<Segment>
Char_Escape_Phrase::analyse(Environ& env, Interp) const
{
    char c;
    if (location().token().kind_ == Token::k_string_newline)
        c = '\n';
    else {
        assert(location().token().kind_ == Token::k_char_escape);
        auto r = location().range();
        assert(r.size() == 2 && r[1] == '_');
        c = r[0];
    }
    return make<Literal_Segment>(share(*this), make_string(&c, 1));
}
Shared<Segment>
Ident_Segment_Phrase::analyse(Environ& env, Interp) const
{
    return make<Ident_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Paren_Segment_Phrase::analyse(Environ& env, Interp) const
{
    return make<Paren_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Bracket_Segment_Phrase::analyse(Environ& env, Interp) const
{
    return make<Bracket_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Segment>
Brace_Segment_Phrase::analyse(Environ& env, Interp) const
{
    return make<Brace_Segment>(share(*this), analyse_op(*expr_, env));
}
Shared<Meaning>
String_Phrase_Base::analyse(Environ& env, Interp) const
{
    return analyse_string(env);
}
Shared<String_Expr>
String_Phrase_Base::analyse_string(Environ& env) const
{
    std::vector<Shared<Segment>> ops;
    for (Shared<const Segment_Phrase> seg : *this)
        ops.push_back(seg->analyse(env, Interp::expr()));
    return String_Expr::make_elements(ops, this);
}

Shared<Meaning>
Unary_Phrase::analyse(Environ& env, Interp) const
{
    switch (op_.kind_) {
    case Token::k_bang:
        env.sstate_.deprecate(&Source_State::not_deprecated_, 1,
            At_Phrase(*this, env),
            "'!a' is deprecated. Use 'not(a)' instead.");
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
    case Token::k_var:
        throw Exception(At_Token(op_, *this, env), "syntax error");
    default:
        die("Unary_Phrase::analyse: bad operator token type");
    }
}

Shared<Meaning>
Local_Phrase::analyse(Environ& env, Interp) const
{
    throw Exception(At_Phrase(*this, env),
        "a local definition must be followed by '; <statement>'");
}
Shared<Definition>
Local_Phrase::as_definition(Environ& env, Fail fl) const
{
    FAIL(fl, nullptr, At_Phrase(*this, env),
        "Not a recursive definition.\n"
        "The syntax 'local <definition>' is a statement.\n"
        "Omit the word 'local' to convert to a definition.");
}

Shared<Meaning>
Include_Phrase::analyse(Environ& env, Interp terp) const
{
    if (terp.is_expr()) {
        throw Exception(At_Phrase(*this, env),
          "Not an expression.\n"
          "The syntax 'include <record>' is a definition, not an expression.\n"
          "Omit the word 'include' to convert this to an expression.");
    } else {
        throw Exception(At_Phrase(*this, env),
          "Not a statement.\n"
          "The syntax 'include <record>' is a definition, not an statement.\n"
          "Try 'local include <record>' instead.");
    }
}
Shared<Definition>
Include_Phrase::as_definition(Environ& env, Fail) const
{
    return make<Include_Definition>(share(*this), arg_);
}

Shared<Meaning>
Test_Phrase::analyse(Environ& env, Interp terp) const
{
    if (terp.is_expr()) {
        throw Exception(At_Phrase(*this, env),
          "Not an expression.");
    } else {
        throw Exception(At_Phrase(*this, env),
          "Not a statement.\n"
          "The syntax 'test <statement>' is a definition, not an statement.\n"
          "Try 'local test <statement>' instead.\n"
          "Or omit the word 'test' to convert to a statement.");
    }
}
Shared<Definition>
Test_Phrase::as_definition(Environ& env, Fail) const
{
    return make<Test_Definition>(share(*this), arg_);
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

    auto pattern = make_pattern(*left, scope);
    pattern->analyse(scope);
    pattern->add_to_scope(scope, 0);
    auto expr = analyse_op(*right, scope, Interp::expr());
    auto nonlocals = make<Enum_Module_Expr>(src,
        move(scope.nonlocal_dictionary_),
        move(scope.nonlocal_exprs_));

    return make<Lambda_Expr>(
        src, pattern, expr, nonlocals, scope.frame_maxslots_);
}

Shared<Meaning>
Lambda_Phrase::analyse(Environ& env, Interp) const
{
    return analyse_lambda(env, share(*this), shared_nonlocals_, left_, right_);
}

Shared<Meaning>
analyse_assoc(Environ& env,
    const Phrase& src, const Phrase& left, Shared<Phrase> right)
{
    if (auto call = dynamic_cast<const Call_Phrase*>(&left)) {
        if (call->is_juxta()) {
            return analyse_assoc(env, src, *call->function_,
                make<Lambda_Phrase>(call->arg_, Token(), right));
        }
    }

    Shared<Operation> right_expr = analyse_op(*right, env);

    if (auto id = dynamic_cast<const Identifier*>(&left))
        return make<Assoc>(share(src), Symbol_Expr{share(*id)}, right_expr);
    if (auto string = dynamic_cast<const String_Phrase*>(&left)) {
        env.sstate_.deprecate(&Source_State::string_colon_deprecated_, 1,
            At_Phrase(src, env),
            "\"name\":value is deprecated.\n"
            "Use 'name':value or [#name,value] instead.");
        auto string_expr = string->analyse_string(env);
        return make<Assoc>(share(src), Symbol_Expr{string_expr}, right_expr);
    }

    throw Exception(At_Phrase(left, env), "invalid definiendum");
}

// Analyse one item in a compound statement, or in the head of a `do` expr,
// which could be either a statement or a local definition.
Shared<Operation>
analyse_stmt(Shared<const Phrase> stmt, Scope& scope, Interp terp)
{
    stmt = strip_parens(stmt);
    if (auto local = cast<const Local_Phrase>(stmt)) {
        auto defn = local->arg_->as_definition(scope, Fail::hard);
        defn->analyse_sequential(scope);
        return defn->add_to_sequential_scope(scope);
    }
    if (auto vardef = cast<const Var_Definition_Phrase>(stmt)) {
        scope.sstate_.deprecate(&Source_State::var_deprecated_, 1,
            At_Phrase(*vardef, scope),
            "'var pattern := expr' is deprecated.\n"
            "Use 'local pattern = expr' instead.");
        auto pat = make_pattern(*vardef->left_, scope);
        pat->analyse(scope);
        auto expr = analyse_op(*vardef->right_, scope, terp);
        pat->add_to_scope(scope, 0);
        return make<Data_Setter>(stmt, slot_t(-1), pat, expr);
    }
    return analyse_op(*stmt, scope, terp);
}

Shared<Meaning>
analyse_do(
    Environ& env,
    Shared<const Phrase> syntax,
    Shared<const Phrase> stmts,
    Shared<const Phrase> bodysrc,
    Interp terp)
{
    Scope scope(env);
    terp = terp.deepen();
    
    Shared<Compound_Op> actions;
    stmts = strip_parens(stmts);
    if (isa<const Empty_Phrase>(stmts))
        actions = Compound_Op::make(0, stmts);
    else if (auto commas = cast<const Comma_Phrase>(stmts)) {
        throw Exception(
            At_Token(commas->args_.front().separator_, *stmts, env),
            "syntax error");
    }
    else if (auto semis = cast<const Semicolon_Phrase>(stmts)) {
        actions = Compound_Op::make(semis->args_.size(), stmts);
        for (size_t i = 0; i < semis->args_.size(); ++i) {
            actions->at(i) =
                analyse_stmt(semis->args_[i].expr_, scope, terp.to_stmt());
        }
    }
    else {
        actions = Compound_Op::make(1, stmts);
        actions->at(0) = analyse_stmt(stmts, scope, terp.to_stmt());
    }

    auto body = analyse_op(*bodysrc, scope, terp);
    env.frame_maxslots_ = scope.frame_maxslots_;
    return make<Do_Expr>(syntax, actions, body);
}

// Analyse a let or where phrase.
Shared<Meaning>
analyse_block(
    Environ& env,
    Shared<const Phrase> syntax,
    Shared<Phrase> bindings,
    Shared<const Phrase> bodysrc,
    Interp terp)
{
    Shared<Definition> adef = bindings->as_definition(env, Fail::hard);
    Recursive_Scope rscope(env, false, adef->syntax_);
    terp = terp.deepen();
    rscope.analyse(*adef);
    auto body = analyse_op(*bodysrc, rscope, terp);
    env.frame_maxslots_ = rscope.frame_maxslots_;
    return make<Block_Op>(syntax,
        move(rscope.executable_), move(body));
}

Shared<Meaning>
Let_Phrase::analyse(Environ& env, Interp terp) const
{
    if (let_.kind_ == Token::k_parametric) {
        auto ctor = analyse_lambda(env, share(*this), false,
            make<Brace_Phrase>(let_, bindings_, in_),
            body_);
        return make<Parametric_Expr>(share(*this), ctor);
    }
    if (let_.kind_ == Token::k_do) {
        return analyse_do(env, share(*this), bindings_, body_, terp);
    }
    if (let_.kind_ == Token::k_let) {
        return analyse_block(env, share(*this), bindings_, body_, terp);
    }
    die("Let_Phrase::analyse: bad kind_");
}

Shared<Meaning>
Where_Phrase::analyse(Environ& env, Interp terp) const
{
    Shared<const Phrase> syntax = share(*this);
    Shared<Phrase> bindings = right_;
    Shared<const Phrase> bodysrc = left_;

    env.sstate_.deprecate(
        &Source_State::where_deprecated_, 1,
        At_Phrase(*syntax, env),
        "'where' is deprecated. Use 'let' instead.");

    // Given 'let bindings1 in body where bindings2',
    // body is analysed in a scope that combines bindings1 and bindings2.
    auto let = cast<const Let_Phrase>(bodysrc);
    if (let && let->let_.kind_ == Token::k_let)
    {
        auto adef1 = let->bindings_->as_definition(env, Fail::hard);
        auto adef2 = bindings->as_definition(env, Fail::hard);
        Recursive_Scope rscope(env, false, syntax);
        adef1->add_to_recursive_scope(rscope);
        adef2->add_to_recursive_scope(rscope);
        rscope.analyse();
        terp = terp.deepen();
        auto body = analyse_op(*let->body_, rscope, terp);
        env.frame_maxslots_ = rscope.frame_maxslots_;
        return make<Block_Op>(syntax,
            move(rscope.executable_), move(body));
    }
    if (auto p = cast<const Paren_Phrase>(bindings))
        bindings = p->body_;
    return analyse_block(env, syntax, bindings, bodysrc, terp);
}

Shared<Meaning>
Binary_Phrase::analyse(Environ& env, Interp) const
{
    // warn about ambiguous syntax: 'f -1' parses as 'f - 1'
    if (op_.kind_ == Token::k_plus || op_.kind_ == Token::k_minus) {
        if (op_.first_white_ != op_.first_) { // space before op
            auto rtok = right_->location().token();
            if (rtok.first_white_ == rtok.first_) { // no space before right
                env.sstate_.system_.warning(
                    Exception(At_Phrase(*this, env),
                      "ambiguous syntax\n"
                      "An expression like 'f -1' parses as 'f - 1'.\n"
                      "You should instead write 'f (-1)' or 'f - 1'."));
            }
        }
    }
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
    case Token::k_plus_plus:
        return make<Cat_Expr>(
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

Shared<Meaning> Apply_Lens_Phrase::analyse(Environ& env, Interp) const
{
    env.sstate_.deprecate(
        &Source_State::at_deprecated_, 1,
        At_Phrase(*this, env),
        "'a@i' is deprecated, use 'a.[i]' instead.");

    return make<Index_Expr>(
        share(*this),
        analyse_op(*arg_, env),
        analyse_op(*function_, env));
}

Shared<Meaning> Dot_Phrase::analyse(Environ& env, Interp) const
{
    // warn about ambiguous syntax: 'f a.b' which parses as '(f a).b'
    if (op_.first_white_ == op_.first_) { // no space before dot
        auto call = cast<Call_Phrase>(left_);
        if (call && call->is_juxta()) { // left of dot is 'f arg'
            auto argtok = call->arg_->location().token();
            if (argtok.first_white_ != argtok.first_) { // space before arg
                env.sstate_.system_.warning(
                    Exception(At_Phrase(*this, env),
                        "ambiguous syntax\n"
                        "An expression like 'f a.b' parses as '(f a).b'.\n"
                        "You should instead write 'f (a.b)' or 'f a .b'."));
            }
        }
    }
    // analyse
    if (auto id = cast<const Identifier>(right_)) {
        return make<Dot_Expr>(
            share(*this),
            analyse_op(*left_, env),
            Symbol_Expr{id});
    }
    if (auto string = cast<const String_Phrase>(right_)) {
        env.sstate_.deprecate(
            &Source_State::dot_string_deprecated_, 1,
            At_Phrase(*this, env),
            Source_State::dot_string_deprecated_msg);
        auto str_expr = string->analyse_string(env);
        return make<Dot_Expr>(
            share(*this),
            analyse_op(*left_, env),
            Symbol_Expr{str_expr});
    }
    if (auto brackets = cast<const Bracket_Phrase>(right_)) {
        return make<Slice_Expr>(
            share(*this),
            analyse_op(*left_, env),
            analyse_op(*right_, env));
    }
    throw Exception(At_Phrase(*right_, env),
        "invalid expression after '.'");
}

// A recursive definition is not an operation.
Shared<Meaning>
Recursive_Definition_Phrase::analyse(Environ& env, Interp terp) const
{
    if (terp.is_expr()) {
        throw Exception(At_Phrase(*this, env),
            "Not an expression.\n"
            "The syntax 'a = b' is a definition, not an expression.\n"
            "Try 'a == b' to test for equality.");
    } else {
        throw Exception(At_Phrase(*this, env),
            "Not a statement.\n"
            "The syntax 'a = b' is a recursive definition, not a statement.\n"
            "Try 'a := b' to reassign an existing local variable.\n"
            "Try 'local a = b' to define a new local variable.");
    }
}

Shared<Meaning>
Var_Definition_Phrase::analyse(Environ& env, Interp) const
{
    throw Exception(At_Phrase(*this, env), "not an expression or statement");
}
Shared<Definition>
Var_Definition_Phrase::as_definition(Environ& env, Fail fl) const
{
    FAIL(fl, nullptr, At_Phrase(*this, env),
        "Not a recursive definition.\n"
        "The deprecated syntax 'var a := b' is a statement.\n"
        "Try 'a = b' instead.");
}

struct Locative_Indexes
{
    Unique<const Locative> locative_;
    std::vector<Shared<const Operation>> indexes_;
};
Unique<const Locative> analyse_locative(Shared<const Phrase>, Environ&, Interp);
void analyse_indexed_locative(
    Shared<const Phrase> ph, Locative_Indexes& il, Environ& env, Interp terp)
{
    ph = strip_parens(ph);
    if (auto dot = cast<const Dot_Phrase>(ph)) {
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
    else {
        il.locative_ = analyse_locative(ph, env, terp);
    }
}
Unique<const Locative> analyse_locative(
    Shared<const Phrase> ph, Environ& env, Interp terp)
{
    ph = strip_parens(ph);
    if (auto id = cast<const Identifier>(ph))
        return env.lookup_lvar(*id, terp.edepth());
    if (auto list = cast<const Bracket_Phrase>(ph)) {
        std::vector<Unique<const Locative>> locs;
        each_item(*list->body_, [&](const Phrase& item)->void {
            locs.push_back(analyse_locative(share(item), env, terp));
        });
        return make_unique<List_Locative>(ph, move(locs));
    }
    if (isa<const Dot_Phrase>(ph) || isa<const Apply_Lens_Phrase>(ph)) {
        Locative_Indexes il;
        analyse_indexed_locative(ph, il, env, terp);
        Shared<const Operation> index;
        if (il.indexes_.size() == 1)
            index = il.indexes_.front();
        else
            index = make<TPath_Expr>(ph, il.indexes_);
        return make_unique<Indexed_Locative>(ph, move(il.locative_), index);
    }
    throw Exception(At_Phrase(*ph, env), "not a locative");
}
Shared<Meaning>
Assignment_Phrase::analyse(Environ& env, Interp terp) const
{
    auto loc = analyse_locative(left_, env, terp);
    auto right = analyse_op(*right_, env);
    return make<Assignment_Action>(share(*this), move(loc), right);
}

void analyse_mutate(
    Shared<const Phrase> ph,
    Unique<const Locative>& loc,
    std::vector<Mutate_Action::XForm>& tx,
    Environ& env,
    Interp terp)
{
    if (auto muph = cast<const Mutate_Phrase>(ph)) {
        analyse_mutate(muph->arg_, loc, tx, env, terp);
        auto func = analyse_op(*muph->function_, env);
        tx.emplace_back(Mutate_Action::XForm{ph, func});
    } else
        loc = analyse_locative(ph, env, terp);
}
Shared<Meaning>
Mutate_Phrase::analyse(Environ& env, Interp terp) const
{
    Unique<const Locative> loc = nullptr;
    std::vector<Mutate_Action::XForm> tx;
    analyse_mutate(share(*this), loc, tx, env, terp);
    return make<Mutate_Action>(share(*this), move(loc), move(tx));
}

Shared<Definition>
as_definition_iter(
    Environ& env, Shared<const Phrase> syntax,
    Phrase& left, Shared<Phrase> right)
{
    if (auto id = dynamic_cast<const Identifier*>(&left)) {
        auto lambda = cast<Lambda_Phrase>(right);
        if (lambda)
            return make<Function_Definition>(move(syntax),
                share(*id), move(lambda));
        else
            return make<Data_Definition>(move(syntax),
                make_pattern(*id,env), move(right));
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        if (call->is_juxta())
            return as_definition_iter(env, move(syntax), *call->function_,
                make<Lambda_Phrase>(call->arg_, Token(), right));
    return make<Data_Definition>(move(syntax),
        make_pattern(left,env), move(right));
}
Shared<Definition>
Recursive_Definition_Phrase::as_definition(Environ& env, Fail) const
{
    return as_definition_iter(env, share(*this), *left_, right_);
}

Shared<Meaning>
Listing_Phrase::analyse(Environ& env, Interp terp) const
{
    Scope scope(env);
    terp = terp.deepen();
    Shared<Compound_Op> compound = Compound_Op::make(args_.size(), share(*this));
    for (size_t i = 0; i < args_.size(); ++i)
        compound->at(i) = analyse_stmt(args_[i].expr_, scope, terp.to_stmt());
    env.frame_maxslots_ = scope.frame_maxslots_;
    return compound;
}

Shared<Definition>
Listing_Phrase::as_definition(Environ& env, Fail fl) const
{
    Shared<Compound_Definition> compound =
        Compound_Definition::make(args_.size(), share(*this));
    size_t j = 0;
    bool contains_statement = false;
    for (size_t i = 0; i < args_.size(); ++i) {
        auto phrase = args_[i].expr_;
        if (isa<Empty_Phrase>(phrase)) continue;
        auto def = phrase->as_definition(env, fl);
        if (def) {
            if (contains_statement) {
                // definition following statement
                throw Exception(At_Phrase(*phrase, env),
                    "definitions cannot be mixed with statements");
            }
            compound->at(j) = def;
            ++j;
        } else if (j > 0) {
            // statement following definition
            throw Exception(At_Phrase(*phrase, env),
                "statements cannot be mixed with definitions");
        } else {
            // first phrase is a statement
            contains_statement = true;
        }
    }
    if (contains_statement)
        return nullptr;
    else {
        compound->resize(j);
        return compound;
    }
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
Paren_Phrase::analyse(Environ& env, Interp terp) const
{
    if (cast<const Empty_Phrase>(body_)) {
        if (terp.is_expr()) {
            env.sstate_.deprecate(
                &Source_State::paren_empty_list_deprecated_, 1,
                At_Phrase(*this, env),
                "Using '()' as the empty list is deprecated. "
                "Use '[]' instead.");
        }
        return Paren_List_Expr::make(0, share(*this));
    }
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        if (terp.is_expr()) {
            env.sstate_.deprecate(&Source_State::paren_list_deprecated_, 1,
                At_Phrase(*this, env),
                "Using '(a,b,c)' as a list is deprecated. "
                "Use '[a,b,c]' instead.");
        }
        auto& items = commas->args_;
        Shared<Paren_List_Expr> list =
            Paren_List_Expr::make(items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env);
        list->init();
        return list;
    } else {
        // One of the few places we directly call Phrase::analyse().
        // The result can be an operation or a metafunction.
        return body_->analyse(env, terp);
    }
}
Shared<Definition>
Paren_Phrase::as_definition(Environ& env, Fail fl) const
{
    return body_->as_definition(env, fl);
}

Shared<Meaning>
Bracket_Phrase::analyse(Environ& env, Interp) const
{
    // TODO: The interpreter works if I just call analyse_op(*body_,...),
    // but the SubCurv compiler currently relies on seeing List_Expr
    // expressions where expression elements are stored separately
    // when comma syntax is used.
    if (cast<const Empty_Phrase>(body_))
        return make_tail_array<List_Expr>(0, share(*this));
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&*body_)) {
        auto& items = commas->args_;
        Shared<List_Expr> list = make_tail_array<List_Expr>
            (items.size(), share(*this));
        for (size_t i = 0; i < items.size(); ++i)
            (*list)[i] = analyse_op(*items[i].expr_, env, Interp::stmt());
        list->init();
        return list;
    } else {
        Shared<List_Expr> list = make_tail_array<List_Expr>(1, share(*this));
        (*list)[0] = analyse_op(*body_, env, Interp::stmt());
        list->init();
        return list;
    }
}

Shared<Meaning>
Call_Phrase::analyse(Environ& env, Interp) const
{
    // warn about ambiguous syntax: 'f a(b)' parses as '(f a)(b)'
    if (is_juxta()) {
        auto argtok = arg_->location().token();
        if (argtok.first_white_ == argtok.first_) { // no space before arg
            auto call = cast<Call_Phrase>(function_);
            if (call && call->is_juxta()) { // function is 'f a'
                auto atok = call->arg_->location().token();
                if (atok.first_white_ != atok.first_) { // space before a
                    env.sstate_.system_.warning(
                        Exception(At_Phrase(*this, env),
                          "ambiguous syntax\n"
                          "An expression like 'f a[b]' parses as '(f a)[b]'.\n"
                          "You should instead write 'f (a[b])' or 'f a [b]'."));
                }
            }
        }
    }
    return function_->analyse(env, Interp::expr())->call(*this, env);
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
Program_Phrase::analyse(Environ& env, Interp terp) const
{
    return body_->analyse(env, terp);
}
Shared<Definition>
Program_Phrase::as_definition(Environ& env, Fail fl) const
{
    return body_->as_definition(env, fl);
}

Shared<Meaning>
Brace_Phrase::analyse(Environ& env, Interp terp) const
{
    if (auto adef = body_->as_definition(env, Fail::soft))
        return analyse_module(*adef, env);
    auto fields = analyse_op(*body_, env, Interp::stmt());
    return make<Record_Expr>(share(*this), fields);
}

Shared<Meaning>
If_Phrase::analyse(Environ& env, Interp terp) const
{
    if (else_expr_ == nullptr) {
        return make<If_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_op(*then_expr_, env, terp));
    } else {
        return make<If_Else_Op>(
            share(*this),
            analyse_op(*condition_, env),
            analyse_op(*then_expr_, env, terp),
            analyse_op(*else_expr_, env, terp));
    }
}

Shared<Meaning>
For_Phrase::analyse(Environ& env, Interp terp) const
{
    Scope scope(env);
    terp = terp.deepen();

    auto pat = make_pattern(*pattern_, env);
    pat->analyse(env);
    auto list = analyse_op(*listexpr_, env);
    pat->add_to_scope(scope, 0);
    Shared<Operation> cond;
    if (condition_)
        cond = analyse_op(*condition_, scope, terp.to_expr());
    auto body = analyse_op(*body_, scope, terp.to_stmt());

    env.frame_maxslots_ = scope.frame_maxslots_;
    return make<For_Op>(share(*this), pat, list, cond, body);
}

Shared<Meaning>
While_Phrase::analyse(Environ& env, Interp terp) const
{
    auto cond = analyse_op(*args_, env);
    auto body = analyse_op(*body_, env, terp.to_stmt());
    return make<While_Op>(share(*this), cond, body);
}

Shared<Meaning>
Parametric_Phrase::analyse(Environ& env, Interp) const
{
    auto ctor = analyse_lambda(env, share(*this), false, param_, body_);
    return make<Parametric_Expr>(share(*this), ctor);
}

Shared<Meaning>
Range_Phrase::analyse(Environ& env, Interp) const
{
    return make<Range_Expr>(
        share(*this),
        analyse_op(*first_, env),
        analyse_op(*last_, env),
        step_ ? analyse_op(*step_, env) : nullptr,
        op1_.kind_ == Token::k_open_range);
}

Shared<Meaning>
Predicate_Assertion_Phrase::analyse(Environ& env, Interp) const
{
    return make<Predicate_Assertion_Expr>(
        share(*this),
        analyse_op(*arg_, env),
        analyse_op(*function_, env));
}

} // namespace curv
