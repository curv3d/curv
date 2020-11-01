// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/pattern.h>

#include <libcurv/context.h>
#include <libcurv/definition.h>
#include <libcurv/exception.h>
#include <libcurv/phrase.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>

namespace curv {

// _ matches any value, discards the value
struct Skip_Pattern : public Pattern
{
    Skip_Pattern(Shared<const Phrase> s) : Pattern(s) {}

    virtual void analyse(Environ&) override
    {
    }
    virtual void add_to_scope(Scope&, unsigned unitno) override
    {
    }
    virtual void exec(Value*, Value, const Context&, Frame&)
    const override
    {
    }
    virtual bool try_exec(Value*, Value, const Context&, Frame&)
    const override
    {
        return true;
    }
    virtual void sc_exec(SC_Value, const Context&, SC_Frame&)
    const override
    {
    }
    virtual void sc_exec(Operation&, SC_Frame&, SC_Frame&)
    const override
    {
    }
};

struct Id_Pattern : public Pattern
{
    slot_t slot_;
    Shared<const Scoped_Variable> var_;

    Id_Pattern(Shared<const Phrase> ph) : Pattern(ph) {}

    virtual void analyse(Environ&) override
    {
    }
    virtual void add_to_scope(Scope& scope, unsigned unitno) override
    {
        auto id = cast<const Identifier>(syntax_);
        auto b = scope.add_binding(id->symbol_, *syntax_, unitno);
        slot_ = b.first;
        var_ = b.second;
    }
    virtual void exec(Value* slots, Value value, const Context&, Frame&)
    const override
    {
        slots[slot_] = value;
    }
    virtual bool try_exec(Value* slots, Value value, const Context&, Frame&)
    const override
    {
        slots[slot_] = value;
        return true;
    }
    virtual void sc_exec(SC_Value value, const Context&, SC_Frame& callee)
    const override
    {
        callee[slot_] = value;
    }
    virtual void sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee)
    const override
    {
        SC_Value val = sc_eval_op(caller, expr);
        if (var_->is_mutable_) {
            // This is a mutable variable, so create a new var and initialize
            // it with val. But, arrays are not supported.
            if (val.type.plex_array_rank() > 0) {
                throw Exception(At_SC_Phrase(expr.syntax_, caller),
                    "mutable array variables are not supported");
            }
            // If I do support mutable array variables, I'll need to use
            // memcpy() for the C++ case.
            SC_Value var = caller.sc_.newvalue(val.type);
            caller.sc_.out() << "  "<<var.type<<" "<<var<<"="<<val<<";\n";
            callee[slot_] = var;
        } else {
            // Immutable variable.
            callee[slot_] = val;
        }
    }
};

struct Const_Pattern : public Pattern
{
    Value value_;

    Const_Pattern(Shared<const Phrase> src, Value val)
    :
        Pattern(src),
        value_(val)
    {}

    virtual void analyse(Environ& env) override
    {
    }
    virtual void add_to_scope(Scope& scope, unsigned unitno) override
    {
    }
    virtual void exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        Ternary ter = value.equal(value_, cx);
        if (ter == Ternary::False)
            throw Exception(cx,
                stringify("argument ",value, " does not equal ", value_));
        if (ter == Ternary::Unknown)
            throw Exception(cx,
                stringify("argument ",value, " cannot be proven equal to ",
                          value_));
    }
    virtual bool try_exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        Ternary ter = value.equal(value_, cx);
        if (ter == Ternary::Unknown)
            throw Exception(cx,
                stringify("argument ",value, " cannot be proven equal to ",
                          value_));
        return ter.to_bool();
    }
};

struct Predicate_Pattern : public Pattern
{
    Shared<Pattern> pattern_;
    Shared<Phrase> predicate_phrase_;
    Shared<Operation> predicate_expr_ = nullptr;

    Predicate_Pattern(Shared<const Call_Phrase> s, Shared<Pattern> p)
    :
        Pattern(s),
        pattern_(p),
        predicate_phrase_(s->function_)
    {}

    inline const Call_Phrase* call_phrase() const
    {
        // This is safe because, by construction, the syntax_ field
        // is initialized from a Call_Phrase. See constructor, above.
        return (Call_Phrase*) &*syntax_;
    }

    bool match(Value arg, Frame& f) const
    {
        static Symbol_Ref callkey = make_symbol("call");
        Value val = predicate_expr_->eval(f);
        Value funv = val;
        for (;;) {
            if (!funv.is_ref())
                throw Exception(At_Phrase(*predicate_phrase_, f),
                    stringify(funv,": not a function"));
            Ref_Value& funp( funv.to_ref_unsafe() );
            switch (funp.type_) {
            case Ref_Value::ty_function:
              {
                Function* fun = (Function*)&funp;
                std::unique_ptr<Frame> f2 {
                    Frame::make(fun->nslots_, f.system_, &f, call_phrase(), nullptr)
                };
                auto result = fun->call(arg, Fail::hard, *f2);
                return result.to_bool(At_Phrase(*call_phrase(), f));
              }
            case Ref_Value::ty_record:
              {
                Record* s = (Record*)&funp;
                if (s->hasfield(callkey)) {
                    funv = s->getfield(callkey, At_Phrase(*call_phrase(), f));
                    continue;
                }
                break;
              }
            }
            throw Exception(At_Phrase(*predicate_phrase_, f),
                stringify(val,": not a function"));
        }
    }

    virtual void analyse(Environ& env) override
    {
        predicate_expr_ = analyse_op(*predicate_phrase_, env);
        pattern_->analyse(env);
    }
    virtual void add_to_scope(Scope& scope, unsigned unitno) override
    {
        pattern_->add_to_scope(scope, unitno);
    }
    virtual void exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        if (!match(value, f))
            throw Exception(cx, stringify("argument ",value, " does not match ",
                predicate_phrase_->location().range()));
        pattern_->exec(slots, value, cx, f);
    }
    virtual bool try_exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        return match(value, f) && pattern_->try_exec(slots, value, cx, f);
    }
};

struct List_Pattern : public Pattern
{
    std::vector<Shared<Pattern>> items_;

    List_Pattern(Shared<const Phrase> s, std::vector<Shared<Pattern>> items)
    :
        Pattern(s),
        items_(std::move(items))
    {}

    virtual void analyse(Environ& env) override
    {
        for (auto& p : items_)
            p->analyse(env);
    }
    virtual void add_to_scope(Scope& scope, unsigned unitno) override
    {
        for (auto& p : items_)
            p->add_to_scope(scope, unitno);
    }
    virtual void exec(Value* slots, Value arg, const Context& argcx, Frame& f)
    const override
    {
        if (is_list(arg)) {
            size_t n = list_count(arg);
            if (items_.size() == n) {
                for (size_t i = 0; i < n; ++i) {
                    items_[i]->exec(
                        slots,
                        list_elem(arg, i, At_Phrase(*items_[i]->syntax_,f)),
                        At_Index(i, argcx), f);
                }
                return;
            }
        }
        throw Exception(argcx,
            stringify(arg," is not a list of ",items_.size()," items"));
    }
    virtual bool try_exec(Value* slots, Value val, const Context& cx, Frame& f)
    const override
    {
        if (!is_list(val)) return false;
        size_t n = list_count(val);
        if (items_.size() != n) return false;
        for (size_t i = 0; i < n; ++i) {
            bool matched = items_[i]->try_exec(
                slots,
                list_elem(val, i, At_Phrase(*items_[i]->syntax_,f)),
                At_Index(i, cx),
                f);
            if (!matched) return false;
        }
        return true;
    }

    virtual void sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee)
    const override
    {
        if (auto list = dynamic_cast<List_Expr*>(&expr)) {
            if (list->size() != items_.size()) {
                throw Exception(At_SC_Phrase(expr.syntax_, caller),
                    stringify("list pattern: expected ",items_.size(),
                        " items, got ",list->size()));
            }
            for (size_t i = 0; i < items_.size(); ++i)
                items_[i]->sc_exec(*list->at(i), caller, callee);
        } else {
            this->sc_exec(
                sc_eval_op(caller, expr),
                At_SC_Phrase(expr.syntax_, callee),
                callee);
        }
    }
    virtual void
    sc_exec(SC_Value val, const Context& valcx, SC_Frame& callee)
    const override
    {
        if (!val.type.is_vec())
            throw Exception(valcx, stringify(
                "list pattern: argument is not a vector (got type ",
                val.type,")"));
        if (val.type.count() != items_.size())
            throw Exception(valcx,
                stringify("list pattern: expected ",items_.size(),
                    " items, got ",val.type.count()));
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->sc_exec(sc_vec_element(callee, val, i), valcx, callee);
    }
};

struct Record_Pattern : public Pattern
{
    struct Field
    {
        Shared<const Phrase> syntax_;
        Shared<Pattern> pat_;
        Shared<const Phrase> dsrc_;
        Shared<Operation> dexpr_;
        Shared<const Phrase> fieldname_;
        Field(Shared<const Phrase> syntax, Shared<Pattern> p,
            Shared<const Phrase> d, Shared<const Phrase> n)
        : syntax_(std::move(syntax)), pat_(p), dsrc_(d), fieldname_(n) {}
        Field() {}
    };
    Symbol_Map<Field> fields_;

    Record_Pattern(Shared<const Phrase> s, Symbol_Map<Field> fields)
    :
        Pattern(s),
        fields_(std::move(fields))
    {}

    virtual void analyse(Environ& env) override
    {
        for (auto& p : fields_) {
            p.second.pat_->analyse(env);
            if (p.second.dsrc_)
                p.second.dexpr_ = analyse_op(*p.second.dsrc_, env);
        }
    }
    virtual void add_to_scope(Scope& scope, unsigned unitno) override
    {
        for (auto& p : fields_)
            p.second.pat_->add_to_scope(scope, unitno);
    }
    virtual void exec(Value* slots, Value value, const Context& valcx, Frame& f)
    const override
    {
        // TODO: Rewrite using a Record iterator.
        auto record = value.to<Record>(valcx);
        auto p = fields_.begin();
        record->each_field(valcx, [&](Symbol_Ref name, Value val)->void {
            while (p != fields_.end()) {
                int cmp = p->first.cmp(name);
                if (cmp < 0) {
                    // record is missing a field in the pattern
                    if (p->second.dexpr_) {
                        auto fval = p->second.dexpr_->eval(f);
                        p->second.pat_->exec(
                            slots, fval, At_Field(p->first.c_str(), valcx), f);
                    } else {
                        throw Exception(valcx, stringify(
                            "record is missing a field named ",p->first));
                    }
                    ++p;
                    continue;
                } else if (cmp == 0) {
                    // matching field in record and pattern
                    auto fval = record->getfield(p->first,valcx);
                    p->second.pat_->exec(
                        slots, fval, At_Field(p->first.c_str(), valcx), f);
                    ++p;
                    return;
                } else
                    break;
            }
            // record has surplus field
            throw Exception(valcx, stringify(
                "record has an unmatched field named ",name));
        });
        while (p != fields_.end()) {
            if (p->second.dexpr_) {
                auto fval = p->second.dexpr_->eval(f);
                p->second.pat_->exec(
                    slots, fval, At_Field(p->first.c_str(), valcx), f);
            } else {
                throw Exception(valcx, stringify(
                    "record is missing a field named ",p->first));
            }
            ++p;
        }
    }
    virtual bool try_exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        // TODO: Rewrite using a Record iterator.
        auto record = value.maybe<Record>();
        if (record == nullptr)
            return false;
        auto p = fields_.begin();
        bool success = true;
        record->each_field(cx, [&](Symbol_Ref name, Value val)->void {
            while (p != fields_.end()) {
                int cmp = p->first.cmp(name);
                if (cmp < 0) {
                    // record is missing a field in the pattern
                    if (p->second.dexpr_) {
                        auto fval = p->second.dexpr_->eval(f);
                        p->second.pat_->try_exec(slots, fval, cx, f);
                    } else {
                        success = false;
                    }
                    ++p;
                    continue;
                } else if (cmp == 0) {
                    // matching field in record and pattern
                    auto fval = record->getfield(p->first,cx);
                    p->second.pat_->try_exec(slots, fval, cx, f);
                    ++p;
                    return;
                } else
                    break;
            }
            success = false;
        });
        while (p != fields_.end()) {
            if (p->second.dexpr_) {
                auto fval = p->second.dexpr_->eval(f);
                p->second.pat_->try_exec(slots, fval, cx, f);
            } else {
                return false;
            }
            ++p;
        }
        return success;
    }
    virtual void sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee)
    const override
    {
        // TODO: implement this
        throw Exception(At_SC_Phrase(syntax_, caller),
            "record patterns are not supported");
    }
};

Symbol_Ref
symbolize(const Phrase& ph, Environ& env)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph))
        return id->symbol_;
    if (auto strph = dynamic_cast<const String_Phrase*>(&ph)) {
        auto val = std_eval(*strph, env);
        auto str = value_to_string(val, Fail::hard, At_Phrase(ph, env));
        return make_symbol(str->data(), str->size());
    }
    throw Exception(At_Phrase(ph, env),
        "not an identifier or string literal");
}

// A helper function while parsing fields in a record pattern.
// Test if a phrase is a pattern that binds exactly 1 identifier,
// which is then used as the fieldname.
// If true, return the identifier, otherwise nullptr.
//
// Within a record pattern, a field pattern is:
//    identifier_pattern
//    identifier_pattern = default_value
//    fieldname : pattern
//    fieldname : pattern = default_value
const Identifier*
identifier_pattern(const Phrase& ph)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph)) {
        if (id->symbol_ == "_")
            return nullptr;
        else
            return id;
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&ph)) {
        if (call->op_.kind_ == Token::k_missing ||
            call->op_.kind_ == Token::k_colon_colon)
        {
            return identifier_pattern(*call->arg_);
        }
    }
    if (auto parens = dynamic_cast<const Paren_Phrase*>(&ph)) {
        return identifier_pattern(*parens->body_);
    }
    return nullptr;
}

Shared<Pattern>
make_pattern(const Phrase& ph, Environ& env)
{
    auto num = dynamic_cast<const Numeral*>(&ph);
    if (num && num->loc_.token().kind_ == Token::k_symbol) {
        return make<Const_Pattern>(share(ph), num->eval());
    }
    if (auto id = dynamic_cast<const Identifier*>(&ph)) {
        if (id->symbol_ == "_" && !id->quoted())
            return make<Skip_Pattern>(share(ph));
        else {
            return make<Id_Pattern>(share(ph));
        }
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&ph)) {
        if (call->op_.kind_ == Token::k_missing) {
            throw Exception(At_Phrase(ph, env),
                "'<predicate> <pattern>' is no longer legal syntax.\n"
                "Use '<pattern> :: <predicate>' instead.");
        }
        if (call->op_.kind_ == Token::k_colon_colon) {
            return make<Predicate_Pattern>(share(*call),
                make_pattern(*call->arg_, env));
        }
    }
    if (auto brackets = dynamic_cast<const Bracket_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        each_item(*brackets->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, env));
        });
        return make<List_Pattern>(share(ph), items);
    }
    if (auto parens = dynamic_cast<const Paren_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        if (isa<Empty_Phrase>(parens->body_)) {
            env.analyser_.deprecate(
                &File_Analyser::paren_empty_list_deprecated_, 1,
                At_Phrase(ph, env),
                "Using '()' as the empty list is deprecated. Use '[]' instead.");
            return make<List_Pattern>(share(ph), items);
        }
        if (isa<Comma_Phrase>(parens->body_)) {
            env.analyser_.deprecate(
                &File_Analyser::paren_list_deprecated_, 2,
                At_Phrase(ph, env),
                "Pattern '(a,b,c)' is deprecated. Use '[a,b,c]' instead.");
            each_item(*parens->body_, [&](Phrase& item)->void {
                items.push_back(make_pattern(item, env));
            });
            return make<List_Pattern>(share(ph), items);
        }
        return make_pattern(*parens->body_, env);
    }
    if (auto braces = dynamic_cast<const Brace_Phrase*>(&ph)) {
        Symbol_Map<Record_Pattern::Field> fields;
        each_item(*braces->body_, [&](Phrase& item)->void {
            if (dynamic_cast<const Empty_Phrase*>(&item))
                return;
            if (auto id = identifier_pattern(item)) {
                auto pat = make_pattern(item, env);
                fields[id->symbol_] = {share(item), pat, nullptr, id};
                return;
            }
            if (auto bin = dynamic_cast<const Binary_Phrase*>(&item)) {
                if (bin->op_.kind_ == Token::k_colon) {
                    Symbol_Ref name = symbolize(*bin->left_, env);
                    Shared<Pattern> pat;
                    Shared<Phrase> dfl_src = nullptr;
                    if (auto def = dynamic_cast
                        <const Recursive_Definition_Phrase*>(&*bin->right_))
                    {
                        pat = make_pattern(*def->left_, env);
                        dfl_src = def->right_;
                    } else if (isa<Empty_Phrase>(bin->right_)) {
                        pat = make<Const_Pattern>(bin->right_, Value{true});
                    } else {
                        pat = make_pattern(*bin->right_, env);
                    }
                    fields[name] = {share(item), pat, dfl_src, bin->left_};
                    return;
                }
            }
            if (auto def =
                dynamic_cast<const Recursive_Definition_Phrase*>(&item))
            {
                auto id = identifier_pattern(*def->left_);
                if (id == nullptr)
                    throw Exception(At_Phrase(*def->left_, env),
                        "not an identifier pattern");
                auto pat = make_pattern(*def->left_, env);
                fields[id->symbol_] = {share(item), pat, def->right_, id};
                return;
            }
            throw Exception(At_Phrase(item, env), "not a field pattern");
        });
        return make<Record_Pattern>(share(ph), std::move(fields));
    }
    throw Exception(At_Phrase(ph, env), "not a pattern");
}

void
Pattern::sc_exec(SC_Value val, const Context& valcx, SC_Frame& callee) const
{
    throw Exception(At_SC_Phrase(syntax_, callee),
        "pattern not supported");
}
void
Pattern::sc_exec(Operation& expr, SC_Frame& caller, SC_Frame& callee) const
{
    throw Exception(At_SC_Phrase(syntax_, callee),
        "pattern not supported");
}

Shared<Record>
record_pattern_default_value(const Pattern& pat, Frame& f)
{
    auto rpat = dynamic_cast<const Record_Pattern*>(&pat);
    if (rpat == nullptr)
        throw Exception(At_Phrase(*pat.syntax_,f), "not a record pattern");
    auto drec = make<DRecord>();
    for (auto& i : rpat->fields_) {
        if (i.second.dexpr_)
            drec->fields_[i.first] = i.second.dexpr_->eval(f);
        else
            throw Exception(At_Phrase(*i.second.syntax_,f),
                "field pattern has no default value");
    }
    return {drec};
}

void
record_pattern_each_parameter(
    const Closure& call, System& sys,
    std::function<void(Symbol_Ref, Value, Value, Shared<const Phrase>)> f)
{
    auto frame = Frame::make(call.nslots_, sys, nullptr, nullptr, nullptr);
    auto rpat = dynamic_cast<const Record_Pattern*>(&*call.pattern_);
    if (rpat == nullptr)
        throw Exception(At_Phrase(*call.pattern_->syntax_, *frame),
            "not a record pattern");
    for (auto& i : rpat->fields_) {
        Value pred = missing;
        auto ppat = dynamic_cast<const Predicate_Pattern*>(&*i.second.pat_);
        if (ppat)
            pred = ppat->predicate_expr_->eval(*frame);
        Value val;
        if (i.second.dexpr_)
            val = i.second.dexpr_->eval(*frame);
        else
            throw Exception(At_Phrase(*i.second.syntax_, *frame),
                "field pattern has no default value");
        f(i.first, pred, val, i.second.fieldname_);
    }
}

} // namespace curv
