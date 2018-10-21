// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/pattern.h>
#include <libcurv/phrase.h>
#include <libcurv/definition.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/gl_context.h>

namespace curv {

struct Skip_Pattern : public Pattern
{
    Skip_Pattern(Shared<const Phrase> s) : Pattern(s) {}

    virtual void analyse(Environ&) override
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
    virtual void gl_exec(GL_Value, const Context&, GL_Frame&)
    const override
    {
    }
    virtual void gl_exec(Operation&, GL_Frame&, GL_Frame&)
    const override
    {
    }
};

struct Id_Pattern : public Pattern
{
    slot_t slot_;

    Id_Pattern(Shared<const Phrase> s, slot_t i) : Pattern(s), slot_(i) {}

    virtual void analyse(Environ&) override
    {
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
    virtual void gl_exec(GL_Value value, const Context&, GL_Frame& callee)
    const override
    {
        callee[slot_] = value;
    }
    virtual void gl_exec(Operation& expr, GL_Frame& caller, GL_Frame& callee)
    const override
    {
        GL_Value val = expr.gl_eval(caller);
        // Why am I creating a new var and initializing it with val?
        // This is useless for an immutable binding (the common case), but
        // necessary for a sequential variable that might be reassigned later.
        GL_Value var = caller.gl.newvalue(val.type);
        caller.gl.out << "  "<<var.type<<" "<<var<<"="<<val<<";\n";
        callee[slot_] = var;
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
    virtual void exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        if (!value.equal(value_,cx))
            throw Exception(cx,
                stringify("argument ",value, " does not equal ", value_));
    }
    virtual bool try_exec(Value* slots, Value value, const Context& cx, Frame& f)
    const override
    {
        return value.equal(value_,cx);
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
        static Symbol callkey = "call";
        Value val = predicate_expr_->eval(f);
        Value funv = val;
        for (;;) {
            if (!funv.is_ref())
                throw Exception(At_Phrase(*predicate_phrase_, f),
                    stringify(funv,": not a function"));
            Ref_Value& funp( funv.get_ref_unsafe() );
            switch (funp.type_) {
            case Ref_Value::ty_function:
              {
                Function* fun = (Function*)&funp;
                std::unique_ptr<Frame> f2 {
                    Frame::make(fun->nslots_, f.system_, &f, call_phrase(), nullptr)
                };
                auto result = fun->call(arg, *f2);
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
    virtual void exec(Value* slots, Value val, const Context& valcx, Frame& f)
    const override
    {
        auto list = val.to<List>(valcx);
        list->assert_size(items_.size(), valcx);
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->exec(slots, list->at(i), At_Index(i, valcx), f);
    }
    virtual bool try_exec(Value* slots, Value val, const Context& cx, Frame& f)
    const override
    {
        auto list = val.dycast<List>();
        if (list == nullptr)
            return false;
        if (list->size() != items_.size())
            return false;
        for (size_t i = 0; i < items_.size(); ++i)
            if (!items_[i]->try_exec(slots, list->at(i), cx, f))
                return false;
        return true;
    }
    virtual void gl_exec(Operation& expr, GL_Frame& caller, GL_Frame& callee)
    const override
    {
        if (auto list = dynamic_cast<List_Expr*>(&expr)) {
            if (list->size() != items_.size()) {
                throw Exception(At_GL_Phrase(expr.syntax_, caller),
                    stringify("list pattern: expected ",items_.size(),
                        " items, got ",list->size()));
            }
            for (size_t i = 0; i < items_.size(); ++i)
                items_[i]->gl_exec(*list->at(i), caller, callee);
        } else {
            this->gl_exec(
                expr.gl_eval(caller),
                At_GL_Phrase(expr.syntax_, callee),
                callee);
        }
    }
    virtual void
    gl_exec(GL_Value val, const Context& valcx, GL_Frame& callee)
    const override
    {
        if (!gl_type_is_vec(val.type))
            throw Exception(valcx, "list pattern: argument is not a vector");
        if (gl_type_count(val.type) != items_.size())
            throw Exception(valcx,
                stringify("list pattern: expected ",items_.size(),
                    " items, got ",gl_type_count(val.type)));
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->gl_exec(gl_vec_element(callee, val, i), valcx, callee);
    }
};

struct Record_Pattern : public Pattern
{
    struct Field
    {
        Shared<Pattern> pat_; Shared<Phrase> dsrc_; Shared<Operation> dexpr_;
        Field(Shared<Pattern> p, Shared<Phrase> d) : pat_(p), dsrc_(d) {}
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
    virtual void exec(Value* slots, Value value, const Context& valcx, Frame& f)
    const override
    {
        // TODO: Rewrite using a Record iterator.
        auto record = value.to<Record>(valcx);
        auto p = fields_.begin();
        record->each_field(valcx, [&](Symbol name, Value val)->void {
            while (p != fields_.end()) {
                int cmp = p->first.cmp(name);
                if (cmp < 0) {
                    // record is missing a field in the pattern
                    if (p->second.dexpr_) {
                        auto fval = p->second.dexpr_->eval(f);
                        p->second.pat_->exec(
                            slots, fval, At_Field(p->first.data(), valcx), f);
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
                        slots, fval, At_Field(p->first.data(), valcx), f);
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
                    slots, fval, At_Field(p->first.data(), valcx), f);
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
        auto record = value.dycast<Record>();
        if (record == nullptr)
            return false;
        auto p = fields_.begin();
        bool success = true;
        record->each_field(cx, [&](Symbol name, Value val)->void {
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
    virtual void gl_exec(Operation& expr, GL_Frame& caller, GL_Frame& callee)
    const override
    {
        // TODO: implement this
        throw Exception(At_GL_Phrase(syntax_, caller),
            "record patterns not supported by Geometry Compiler");
    }
};

Symbol
symbolize(const Phrase& ph, Scope& scope)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph))
        return id->symbol_;
    if (auto strph = dynamic_cast<const String_Phrase*>(&ph)) {
        auto val = std_eval(*strph, scope);
        auto str = val.to<const String>(At_Phrase(ph, scope));
        return Symbol{str};
    }
    throw Exception(At_Phrase(ph, scope),
        "not an identifier or string literal");
}

Shared<Pattern>
make_pattern(const Phrase& ph, Scope& scope, unsigned unitno)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph)) {
        if (id->symbol_ == Symbol{"_"})
            return make<Skip_Pattern>(share(ph));
        else {
            slot_t slot = scope.add_binding(id->symbol_, ph, unitno);
            return make<Id_Pattern>(share(ph), slot);
        }
    }
    if (auto call = dynamic_cast<const Call_Phrase*>(&ph)) {
        if (call->op_.kind_ == Token::k_missing)
            return make<Predicate_Pattern>(share(*call),
                make_pattern(*call->arg_, scope, unitno));
    }
    if (auto brackets = dynamic_cast<const Bracket_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        each_item(*brackets->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, scope, unitno));
        });
        return make<List_Pattern>(share(ph), items);
    }
    if (auto parens = dynamic_cast<const Paren_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        if (dynamic_cast<const Empty_Phrase*>(&*parens->body_) != nullptr)
            return make<List_Pattern>(share(ph), items);
        if (dynamic_cast<const Comma_Phrase*>(&*parens->body_) == nullptr
         && dynamic_cast<const Semicolon_Phrase*>(&*parens->body_) == nullptr)
            return make_pattern(*parens->body_, scope, unitno);
        each_item(*parens->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, scope, unitno));
        });
        return make<List_Pattern>(share(ph), items);
    }
    if (auto braces = dynamic_cast<const Brace_Phrase*>(&ph)) {
        Symbol_Map<Record_Pattern::Field> fields;
        each_item(*braces->body_, [&](Phrase& item)->void {
            if (dynamic_cast<const Empty_Phrase*>(&item))
                return;
            if (auto id = dynamic_cast<const Identifier*>(&item)) {
                auto pat = make_pattern(*id, scope, unitno);
                fields[id->symbol_] = {pat, nullptr};
                return;
            }
            if (auto bin = dynamic_cast<const Binary_Phrase*>(&item)) {
                if (bin->op_.kind_ == Token::k_colon) {
                    Symbol name = symbolize(*bin->left_, scope);
                    Shared<Pattern> pat;
                    Shared<Phrase> dfl_src = nullptr;
                    if (auto def = dynamic_cast
                        <const Recursive_Definition_Phrase*>(&*bin->right_))
                    {
                        pat = make_pattern(*def->left_, scope, unitno);
                        dfl_src = def->right_;
                    } else if (isa<Empty_Phrase>(bin->right_)) {
                        pat = make<Const_Pattern>(bin->right_, Value{true});
                    } else {
                        pat = make_pattern(*bin->right_, scope, unitno);
                    }
                    fields[name] = {pat, dfl_src};
                    return;
                }
            }
            if (auto def =
                dynamic_cast<const Recursive_Definition_Phrase*>(&item))
            {
                auto id = cast<const Identifier>(def->left_);
                if (id == nullptr)
                    throw Exception(At_Phrase(*def->left_, scope),
                        "not an identifier");
                auto pat = make_pattern(*id, scope, unitno);
                fields[id->symbol_] = {pat, def->right_};
                return;
            }
            throw Exception(At_Phrase(item, scope), "not a field pattern");
        });
        return make<Record_Pattern>(share(ph), std::move(fields));
    }
    throw Exception(At_Phrase(ph, scope), "not a pattern");
}

void
Pattern::gl_exec(GL_Value val, const Context& valcx, GL_Frame& callee) const
{
    throw Exception(At_GL_Phrase(syntax_, callee),
        "pattern not supported by Geometry Compiler");
}
void
Pattern::gl_exec(Operation& expr, GL_Frame& caller, GL_Frame& callee) const
{
    throw Exception(At_GL_Phrase(syntax_, callee),
        "pattern not supported by Geometry Compiler");
}

} // namespace curv
