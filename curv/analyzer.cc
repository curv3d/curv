// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

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

/*
For recursive bindings, we must delay analysis until all statements have
been examined and the def_dictionary is built.

For sequential bindings, it would be more convenient to analyze each statement
as it arrives, using an environment that is extended as each definition arrives.
Actions that arrive before the first definition are analyzed using the parent
environment.

How to implement this?
1. Make one pass through the statements, looking for the first definition.
   Classify the bindings as sequential or recursive.
   Make a second pass through the statements, using different algorithms for
   sequential and recursive bindings.
2. Before the first definition is seen, accumulate actions in action_phrases.
   On the first definition, classify as recursive or sequential.
   For sequential, analyze the cached action phrases, then switch to analyzing
   each statement. For recursive, use the original algorithm.
3. Next step, permit a mix of sequential and recursive definitions, but,
   a recursive definition can't reference a sequential definition in the same
   scope.

   We have to accumulate all of the statement phrases before we do any analysis.
   During the add_statement phase, we:
   * Assign a sequence number to each action and sequential definition.
     At the end we have a seq_count_.
   * Make a list of action phrases, storing seq_no and Phrase in each entry.
   * Make a dictionary mapping name to (slot, seq_no (if sequential), Definition)

   With this implementation, we don't know the sequence# of a recursive
   definition. If it is at the top of a block, we'd expect it to see only
   the parent scope. If it is at the bottom of a block, we'd expect it to
   see sequential definitions (but we want this to be an error). No way to
   conditionally issue the error. I guess that's a limitation for now.
   Recursive definitions are not in the scope of sequential definitions
   within the same block. In that case, do we want to make it an error for
   a recursive definition to follow a sequential definition? Okay, do it.

   During analysis, we:
   * Make a list of action- and seq.defn- actions from the action phrases
     and the dictionary. We know the total # of actions from the previous phase.
     We construct an array of actions of this size. Then we initialize it, out
     of order, by scanning the action phrase list and the dictionary.

   * an array of statements, each with a statement #, classified as action,
     recursive defn or sequential defn.
   * a dictionary mapping each name to a statement #, slot, definition kind
   Or:
   * A list of action phrases, storing stmt# and Phrase in each entry.
   * A dictionary mapping name to (stmt#, slot, Definition)
   During the analysis phase, we'll do this:
   * Traverse the statement list, analyzing each action and definition.
     We'll publish the current statement # and we'll hide sequential definitions
     not yet processed.
   * Order of statement processing doesn't matter.
4. Suppose you mix recursive definitions with sequential *re*-definitions.
   We associate a different slot with each rebinding (each slot is either
   missing (not initialized yet) or initialized once to a value that doesn't
   change. Iteration variables in a while statement have their own slots
   which aren't accessible to recursive functions. The final value of an
   iteration variable is copied to a new slot which doesn't change.
   This is pretty complicated; it's easier to make it illegal to mix
   recursive definitions with sequential re-definitions. Or, a recursive
   definition can't reference a definition that is rebound.
   Or, a recursive definition can't reference a sequential definition in the
   same scope.
5. `m = {a:=f(x), b:=y, ...}; f=...(m.b)...;`
   The module fields are initialized sequentially during module construction.
   When `f` is called during the initialization of `a`, `m` is uninitialized.
   If we use recursive definitions in the module, this works fine.
*/

void
Statement_Analyzer::add_statement(Shared<const Phrase> stmt)
{
    auto def = stmt->analyze_def(*parent_);
    if (def == nullptr)
        action_phrases_.emplace_back(seq_count_++, std::move(stmt));
    else {
        if (def_dictionary_.empty()) {
            kind_ = def->kind_;
        } else if (def->kind_ != kind_) {
            throw Exception(At_Phrase(*stmt, *parent_),
                "can't mix recursive and sequential definitions");
        }

        Atom name = def->name_->atom_;
        if (def_dictionary_.find(name) != def_dictionary_.end())
            throw Exception(At_Phrase(*def->name_, *parent_),
                stringify(name, ": multiply defined"));
        int pos = (def->kind_ == Definition::k_sequential ? seq_count_++ : -1);
        def_dictionary_.emplace(
            std::make_pair(name, Binding{slot_count_++, pos, def}));

        if (kind_ == Definition::k_recursive) {
            auto lambda = dynamic_cast<Lambda_Phrase*>(def->definiens_.get());
            if (lambda != nullptr)
                lambda->shared_nonlocals_ = true;
        }
    }
}

bool
Statement_Analyzer::Binding::is_function_definition()
{
    return isa<const Lambda_Phrase>(def_->definiens_);
}

bool
Statement_Analyzer::Binding::defined_at_position(int pos)
{
    return pos > seq_no_;
}

bool
Statement_Analyzer::Binding::is_recursive()
{
    return seq_no_ < 0;
}

Shared<Meaning>
Statement_Analyzer::single_lookup(const Identifier& id)
{
    auto b = def_dictionary_.find(id.atom_);
    if (b != def_dictionary_.end()) {
        if (b->second.defined_at_position(cur_pos_)) {
            if (kind_ == Definition::k_recursive) {
                if (b->second.is_function_definition())
                    return make<Indirect_Function_Ref>(share(id),
                        statements_.slot_, b->second.slot_, slot_count_);
                else
                    return make<Indirect_Lazy_Ref>(
                        share(id), statements_.slot_, b->second.slot_);
            } else { // sequential
                if (target_is_module_) {
                    return make<Indirect_Strict_Ref>(
                        share(id), statements_.slot_, b->second.slot_);
                } else {
                    return make<Let_Ref>(share(id),
                        b->second.slot_ + parent_->frame_nslots_);
                }
            }
        }
    }
    return nullptr;
}

Shared<Meaning>
Statement_Analyzer::Thunk_Environ::single_lookup(const Identifier& id)
{
    assert(analyzer_.kind_ == Definition::k_recursive);
    auto b = analyzer_.def_dictionary_.find(id.atom_);
    if (b != analyzer_.def_dictionary_.end()) {
        if (b->second.defined_at_position(analyzer_.cur_pos_)) {
            if (b->second.is_function_definition())
                return make<Nonlocal_Function_Ref>(share(id), b->second.slot_);
            else
                return make<Nonlocal_Lazy_Ref>(share(id), b->second.slot_);
        }
    }

    auto n = analyzer_.nonlocal_dictionary_.find(id.atom_);
    if (n != analyzer_.nonlocal_dictionary_.end())
        return make<Nonlocal_Strict_Ref>(share(id), n->second);
    auto m = parent_->lookup(id);
    if (isa<Constant>(m))
        return m;
    if (auto expr = cast<Operation>(m)) {
        slot_t slot = analyzer_.def_dictionary_.size()
            + analyzer_.statements_.nonlocal_exprs_.size();
        analyzer_.nonlocal_dictionary_[id.atom_] = slot;
        analyzer_.statements_.nonlocal_exprs_.push_back(expr);
        return make<Nonlocal_Strict_Ref>(share(id), slot);
    }
    return m;
}

void
Statement_Analyzer::analyze(Shared<const Phrase> source)
{
    if (kind_ == Definition::k_sequential)
        is_sequential_statement_list_ = true;

    statements_.actions_.reserve(seq_count_);
    statements_.actions_.insert(statements_.actions_.end(), seq_count_, nullptr);

    Shared<List> defn_values;
    if (kind_ == Definition::k_sequential && !target_is_module_) {
        statements_.slot_ = (slot_t)(-1);
        frame_nslots_ += def_dictionary_.size();
        defn_values = nullptr;
    } else {
        statements_.slot_ = frame_nslots_++;
        defn_values = make_list(def_dictionary_.size());
    }
    frame_maxslots_ = std::max(frame_nslots_, frame_maxslots_);

    // analyze action phrases
    for (auto& ap : action_phrases_) {
        cur_pos_ = ap.seq_no_;
        statements_.actions_[ap.seq_no_] = analyze_action(*ap.phrase_, *this);
    }

    // analyze definitions
    for (auto& b : def_dictionary_) {
        cur_pos_ = std::max(0, b.second.seq_no_);
        if (kind_ == Definition::k_recursive) {
            // analyze definiens
            Thunk_Environ tenv(*this);
            auto expr = analyze_op(*b.second.def_->definiens_, tenv);

            // construct initial slot value
            if (b.second.is_function_definition()) {
                auto& l = dynamic_cast<Lambda_Expr&>(*expr);
                defn_values->at(b.second.slot_) =
                    {make<Lambda>(l.body_, l.nargs_, l.nslots_)};
            } else
                defn_values->at(b.second.slot_) =
                    {make<Thunk>(expr, tenv.frame_maxslots_)};
        } else { // sequential
            // analyze definiens
            auto expr = analyze_op(*b.second.def_->definiens_, *this);

            // construct initial slot value
            if (target_is_module_) {
                defn_values->at(b.second.slot_) = missing;
                statements_.actions_[b.second.seq_no_] = make<Indirect_Assign>(
                    b.second.def_->source_,
                    statements_.slot_, b.second.slot_,
                    expr);
            } else {
                statements_.actions_[b.second.seq_no_] = make<Let_Assign>(
                    b.second.def_->source_,
                    b.second.slot_ + parent_->frame_nslots_,
                    expr, false);
            }
        }
    }
    statements_.defn_values_ = defn_values;

    parent_->frame_maxslots_ = frame_maxslots_;
    cur_pos_ = seq_count_;
}

Shared<Module::Dictionary>
Statement_Analyzer::make_module_dictionary()
{
    auto dict = make<Module::Dictionary>();
    for (auto& b : def_dictionary_)
        (*dict)[b.first] = b.second.slot_;
    return std::move(dict);
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
        Module::Dictionary nonlocal_dictionary_;
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
            auto n = nonlocal_dictionary_.find(id.atom_);
            if (n != nonlocal_dictionary_.end())
                return make<Nonlocal_Strict_Ref>(share(id), n->second);
            auto m = parent_->lookup(id);
            if (isa<Constant>(m))
                return m;
            if (auto expr = cast<Operation>(m)) {
                slot_t slot = nonlocal_exprs_.size();
                nonlocal_dictionary_[id.atom_] = slot;
                nonlocal_exprs_.push_back(expr);
                return make<Nonlocal_Strict_Ref>(share(id), slot);
            }
            return m;
        }
    };
    Arg_Environ env2(&env, params, shared_nonlocals_);
    auto expr = analyze_op(*right_, env2);
    auto src = share(*this);
    Shared<List_Expr> nonlocals =
        List_Expr::make(env2.nonlocal_exprs_.size(), src);
    // TODO: use some kind of Tail_Array move constructor
    for (size_t i = 0; i < env2.nonlocal_exprs_.size(); ++i)
        (*nonlocals)[i] = env2.nonlocal_exprs_[i];

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
        return make<Let_Assign>(share(*this), let->slot_, expr, true);
    auto indir = cast<Indirect_Strict_Ref>(m);
    if (indir)
        return make<Indirect_Assign>(share(*this),
            indir->slot_, indir->index_, expr);

    // this should never happen
    throw Exception(At_Phrase(*left_, env), "not a sequential variable name");
}

Shared<Definition>
analyze_def_iter(
    Environ& env, Shared<const Phrase> source,
    Phrase& left, Shared<Phrase> right, Definition::Kind kind)
{
    if (auto id = dynamic_cast<const Identifier*>(&left))
        return make<Definition>(std::move(source), share(*id), right, kind);
    if (auto call = dynamic_cast<const Call_Phrase*>(&left))
        return analyze_def_iter(env, std::move(source), *call->function_,
            make<Lambda_Phrase>(call->arg_, Token(), right), kind);
    throw Exception(At_Phrase(left,  env), "invalid definiendum");
}

Shared<Definition>
Recursive_Definition_Phrase::analyze_def(Environ& env) const
{
    return analyze_def_iter(env, share(*this), *left_, right_,
        Definition::k_recursive);
}
Shared<Definition>
Sequential_Definition_Phrase::analyze_def(Environ& env) const
{
    return analyze_def_iter(env, share(*this), *left_, right_,
        Definition::k_sequential);
}

Shared<Meaning>
Semicolon_Phrase::analyze(Environ& env) const
{
    // Blocks support mutually recursive bindings, like let-rec in Scheme.
    Statement_Analyzer analyzer{env, false};
    for (size_t i = 0; i < args_.size() - 1; ++i)
        analyzer.add_statement(args_[i].expr_);
    analyzer.analyze(share(*this));
    analyzer.is_analyzing_action_ = env.is_analyzing_action_;
    auto body = analyze_tail(*args_.back().expr_, analyzer);
    env.frame_maxslots_ = analyzer.frame_maxslots_;
    return make<Block_Op>(share(*this),
        std::move(analyzer.statements_), std::move(body));
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
Program_Phrase::analyze_def(Environ& env) const
{
    return body_->analyze_def(env);
}

/// In the grammar, a <list> phrase is zero or more constituent phrases
/// separated by commas or semicolons.
/// This function iterates over each constituent phrase.
static inline void
each_item(const Phrase& phrase, std::function<void(const Phrase&)> func)
{
    if (dynamic_cast<const Empty_Phrase*>(&phrase))
        return;
    if (auto commas = dynamic_cast<const Comma_Phrase*>(&phrase)) {
        for (auto& i : commas->args_)
            func(*i.expr_);
        return;
    }
    if (auto semis = dynamic_cast<const Semicolon_Phrase*>(&phrase)) {
        for (auto& i : semis->args_)
            func(*i.expr_);
        return;
    }
    func(phrase);
}

Shared<Meaning>
Brace_Phrase::analyze(Environ& env) const
{
    // A brace phrase is:
    //  * empty
    //  * a binder
    //  * a definition
    //  * a comma-separated list of actions and binders
    //  * a semicolon-separated list of actions and definitions

    bool is_module;
    if (isa<Empty_Phrase>(body_)) {
        is_module = false;
    } else if (isa<Comma_Phrase>(body_)) {
        is_module = false;
    } else if (isa<Semicolon_Phrase>(body_)) {
        is_module = true;
    } else {
        is_module = (body_->analyze_def(env) != nullptr);
    }

    auto source = share(*this);

    if (is_module) {
        Statement_Analyzer fields{env, true};
        each_item(*body_, [&](const Phrase& stmt)->void {
            fields.add_statement(share(stmt));
        });
        fields.analyze(source);
        return make<Module_Expr>(source,
            fields.make_module_dictionary(),
            std::move(fields.statements_));
    } else {
        auto record = make<Record_Expr>(source);
        each_item(*body_, [&](const Phrase& item)->void {
            record->fields_.push_back(analyze_op(item, env));
        });
        return record;
    }
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
