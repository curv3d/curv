// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/lens.h>
#include <libcurv/list.h>
#include <libcurv/meaning.h>
#include <libcurv/module.h>
#include <libcurv/parametric.h>
#include <libcurv/prim.h>
#include <libcurv/record.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/string.h>
#include <cmath>

namespace curv {

Value
tail_eval_frame(std::unique_ptr<Frame> f)
{
    while (f->next_op_ != nullptr)
        f->next_op_->tail_eval(f);
    return f->result_;
}

Value
Operation::eval(Frame& f) const
{
    throw Exception(At_Phrase(*syntax_, f), "not an expression");
}

void
Operation::tail_eval(std::unique_ptr<Frame>& f) const
{
    f->result_ = eval(*f);
    f->next_op_ = nullptr;
}

void
Operation::Action_Executor::push_value(Value, const Context& cstmt)
{
    throw Exception(cstmt, "illegal statement type: expecting an action");
}
void
Operation::Action_Executor::push_field(Symbol_Ref, Value, const Context& cstmt)
{
    throw Exception(cstmt, "illegal statement type: expecting an action");
}

void
Operation::List_Executor::push_value(Value val, const Context& cx)
{
    list_.push_back(val);
}
void
Operation::List_Executor::push_field(
    Symbol_Ref name, Value elem, const Context& cstmt)
{
    Shared<List> pair = List::make({name.to_value(),elem});
    list_.push_back(Value(pair));
}

void
Operation::Record_Executor::push_value(Value val, const Context& cstmt)
{
    auto pair = val.to<List>(cstmt);
    pair->assert_size(2, cstmt);
    Symbol_Ref name = value_to_symbol(pair->at(0), cstmt);
    record_.fields_[name] = pair->at(1);
}
void
Operation::Record_Executor::push_field(Symbol_Ref name, Value value, const Context& cx)
{
    record_.fields_[name] = value;
}

void
Just_Expression::exec(Frame& f, Executor& ex) const
{
    ex.push_value(eval(f), At_Phrase(*syntax_,f));
}

void
Null_Action::exec(Frame&, Executor&) const
{
}

Value
Constant::eval(Frame&) const
{
    return value_;
}

Value
Symbolic_Ref::eval(Frame& f) const
{
    auto& m = *f.nonlocals_;
    auto b = m.dictionary_->find(name_);
    assert(b != m.dictionary_->end());
    return m.get(b->second);
}

Value
Module_Data_Ref::eval(Frame& f) const
{
    Module& m = (Module&)f[slot_].to_ref_unsafe();
    assert(m.subtype_ == Ref_Value::sty_module);
    return m.at(index_);
}

Value
Nonlocal_Data_Ref::eval(Frame& f) const
{
    return f.nonlocals_->at(slot_);
}

Value
Local_Data_Ref::eval(Frame& f) const
{
    return f[slot_];
}

Value record_at(Value rec, Symbol_Ref id, const Context& cx)
{
#if 0
    if (auto list = rec.maybe<const List>()) {
        Shared<List> result = List::make(list->size());
        for (unsigned i = 0; i < list->size(); ++i)
            result->at(i) = record_at(list->at(i), id, cx);
        return {result};
    }
    else
#endif
        return rec.at(id, cx);
}
Value
Dot_Expr::eval(Frame& f) const
{
    Value basev = base_->eval(f);
    Symbol_Ref id = selector_.eval(f);
    return record_at(basev, id, At_Phrase(*base_->syntax_, f));
}

#define BOOL_EXPR_EVAL(AND_EXPR, AND, NOT, FALSE) \
Value AND_EXPR::eval(Frame& f) const \
{ \
    Value av = arg1_->eval(f); \
    if (av.is_bool()) { \
        /* fast path */ \
        if (NOT av.to_bool_unsafe()) \
            return {FALSE}; \
        Value bv = arg2_->eval(f); \
        assert_bool(bv, At_Phrase(*arg2_->syntax_, f)); \
        return bv; \
    } \
    /* slow path, handle case where arg1 is a reactive Bool */ \
    assert_bool(av, At_Phrase(*arg1_->syntax_, f)); \
    /* TODO: if arg2_->eval aborts, construct Error value and continue. */ \
    Value bv = arg2_->eval(f); \
    if (bv.is_bool()) { \
        /* The 'return {false}' case is importance for correctness; */ \
        /* see new_core/Reactive "Lazy Boolean Operators" */ \
        bool b = bv.to_bool_unsafe(); \
        if (NOT b) return {FALSE}; else return av; \
    } \
    assert_bool(bv, At_Phrase(*arg2_->syntax_, f)); \
    return {make<Reactive_Expression>( \
        SC_Type::Bool(), \
        make<AND_EXPR>( \
            share(*syntax_), \
            to_expr(av, *arg1_->syntax_), \
            to_expr(bv, *arg2_->syntax_)), \
        At_Phrase(*syntax_, f))}; \
} \
void AND_EXPR::print(std::ostream& out) const \
  { out<<"("<<*arg1_<<#AND<<*arg2_<<")"; }
BOOL_EXPR_EVAL(And_Expr, &&, !, false)
BOOL_EXPR_EVAL(Or_Expr, ||, , true)

Value
If_Op::eval(Frame& f) const
{
    throw Exception{At_Phrase{*syntax_, f},
        "if: not an expression (missing else clause)"};
}
void
If_Op::exec(Frame& f, Executor& ex) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->exec(f, ex);
}

Value
If_Else_Op::eval(Frame& f) const
{
    Value cond = arg1_->eval(f);
    At_Phrase cx(*arg1_->syntax_, f);
    if (cond.is_bool()) {
        if (cond.to_bool(cx))
            return arg2_->eval(f);
        else
            return arg3_->eval(f);
    }
    auto re = cond.maybe<Reactive_Value>();
    if (re && re->sctype_.is_bool()) {
        Value a2 = arg2_->eval(f);
        Value a3 = arg3_->eval(f);
        SC_Type t2 = sc_type_of(a2);
        SC_Type t3 = sc_type_of(a3);
        if (t2 == t3) {
            return {make<Reactive_Expression>(
                t2,
                make<If_Else_Op>(
                    share(*syntax_),
                    to_expr(cond, *arg1_->syntax_),
                    to_expr(a2, *arg2_->syntax_),
                    to_expr(a3, *arg3_->syntax_)),
                At_Phrase(*syntax_, f))};
        }
        throw Exception(At_Phrase(*syntax_, f),
            stringify("then and else expressions have mismatched types: ",
                t2," and ",t3));
    }
    throw Exception(cx, stringify(cond, " is not a boolean"));
}
void
If_Else_Op::tail_eval(std::unique_ptr<Frame>& f) const
{
    Value cond = arg1_->eval(*f);
    At_Phrase cx(*arg1_->syntax_, *f);
    if (cond.is_bool()) {
        if (cond.to_bool(cx))
            f->next_op_ = &*arg2_;
        else
            f->next_op_ = &*arg3_;
        return;
    }
    auto re = cond.maybe<Reactive_Value>();
    if (re && re->sctype_.is_bool()) {
        Value a2 = arg2_->eval(*f);
        Value a3 = arg3_->eval(*f);
        SC_Type t2 = sc_type_of(a2);
        SC_Type t3 = sc_type_of(a3);
        if (t2 == t3) {
            f->result_ = Value{make<Reactive_Expression>(
                t2,
                make<If_Else_Op>(
                    share(*syntax_),
                    to_expr(cond, *arg1_->syntax_),
                    to_expr(a2, *arg2_->syntax_),
                    to_expr(a3, *arg3_->syntax_)),
                At_Phrase(*syntax_, *f))};
            f->next_op_ = nullptr;
            return;
        }
        throw Exception(At_Phrase(*syntax_, *f),
            stringify("then and else expressions have mismatched types: ",
                t2," and ",t3));
    }
    throw Exception(cx, stringify(cond, " is not a boolean"));
}
void
If_Else_Op::exec(Frame& f, Executor& ex) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->exec(f, ex);
    else
        arg3_->exec(f, ex);
}

Value
Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    At_Phrase cx(*syntax_, f);
    return eqval<Equal_Expr>(a.equal(b, cx), a, b, cx);
}
void Equal_Expr::print(std::ostream& out) const
  { out<<"("<<*arg1_<<"=="<<*arg2_<<")"; }
Value
Not_Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    At_Phrase cx(*syntax_, f);
    return eqval<Not_Equal_Expr>(!a.equal(b, cx), a, b, cx);
}
void Not_Equal_Expr::print(std::ostream& out) const
  { out<<"("<<*arg1_<<"!="<<*arg2_<<")"; }

Value Index_Expr::eval(Frame& f) const
{
    return get_value_at_index(arg1_->eval(f), arg2_->eval(f),
        nullptr, nullptr, At_Phrase(*syntax_, f));
}
Value Slice_Expr::eval(Frame& f) const
{
    Value value = arg1_->eval(f);
    Value slice = arg2_->eval(f);
    return get_value_at_boxed_slice(value, slice, At_Phrase(*syntax_, f));
}

Value
call_func(Value func, Value arg, Shared<const Phrase> call_phrase, Frame& f)
{
    static Symbol_Ref callkey = make_symbol("call");
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.to_ref_unsafe() );
        switch (funp.type_) {
        case Ref_Value::ty_function:
          {
            Function* fun = (Function*)&funp;
            std::unique_ptr<Frame> f2 {
                Frame::make(fun->nslots_, f.system_, &f, call_phrase, nullptr)
            };
            f2->func_ = share(*fun);
            fun->tail_call(arg, f2);
            return tail_eval_frame(std::move(f2));
          }
        case Ref_Value::ty_record:
          {
            Record* s = (Record*)&funp;
            if (s->hasfield(callkey)) {
                funv = s->getfield(callkey, At_Phrase(*call_phrase, f));
                continue;
            }
            break;
          }
        case Ref_Value::ty_abstract_list:
        case Ref_Value::ty_reactive:
          {
            return get_value_at_boxed_slice(funv, arg,
                At_Phrase(*call_phrase, f));
          }
        }
        throw Exception(At_Phrase(*func_part(call_phrase), f),
            stringify(func,": not a function"));
    }
}
void
tail_call_func(
    Value func, Value arg,
    Shared<const Phrase> call_phrase, std::unique_ptr<Frame>& f)
{
    static Symbol_Ref callkey = make_symbol("call");
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), *f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.to_ref_unsafe() );
        switch (funp.type_) {
        case Ref_Value::ty_function:
          {
            Function* fun = (Function*)&funp;
            f = Frame::make(
                fun->nslots_, f->system_, f->parent_frame_,
                call_phrase, nullptr);
            f->func_ = share(*fun);
            fun->tail_call(arg, f);
            return;
          }
        case Ref_Value::ty_record:
          {
            Record* s = (Record*)&funp;
            if (s->hasfield(callkey)) {
                funv = s->getfield(callkey, At_Phrase(*call_phrase, *f));
                continue;
            }
            break;
          }
        case Ref_Value::ty_abstract_list:
        case Ref_Value::ty_reactive:
          {
            f->result_ = get_value_at_boxed_slice(funv, arg,
                At_Phrase(*call_phrase, *f));
            f->next_op_ = nullptr;
            return;
          }
        }
        throw Exception(At_Phrase(*func_part(call_phrase), *f),
            stringify(func,": not a function"));
    }
}
Value
Call_Expr::eval(Frame& f) const
{
    return call_func(func_->eval(f), arg_->eval(f), syntax_, f);
}
void
Call_Expr::tail_eval(std::unique_ptr<Frame>& f) const
{
    tail_call_func(func_->eval(*f), arg_->eval(*f), syntax_, f);
}

Value
List_Expr_Base::eval(Frame& f) const
{
    // TODO: If the # of elements generated is known at compile time,
    // then the List could be constructed directly without using a std::vector?
    // The result now depends on the element types, but we could still use
    // the length information to optimize List_Builder.
    List_Builder lb;
    List_Executor lex(lb);
    for (size_t i = 0; i < this->size(); ++i)
        (*this)[i]->exec(f, lex);
    return lb.get_value();
}
void Paren_List_Expr_Base::exec(Frame& f, Executor& ex) const
{
    for (size_t i = 0; i < this->size(); ++i)
        this->at(i)->exec(f, ex);
}

void
Spread_Op::exec(Frame& f, Executor& ex) const
{
    At_Phrase cstmt(*syntax_, f);
    At_Phrase carg(*arg_->syntax_, f);
    auto arg = arg_->eval(f);
    if (auto list = arg.maybe<const List>()) {
        for (size_t i = 0; i < list->size(); ++i)
            ex.push_value(list->at(i), cstmt);
        return;
    }
    if (auto string = arg.maybe<const String>()) {
        for (char c : *string)
            ex.push_value(Value{c}, cstmt);
        return;
    }
    if (auto rec = arg.maybe<const Record>()) {
        for (auto i = rec->iter(); !i->empty(); i->next())
            ex.push_field(i->key(), i->value(carg), cstmt);
        return;
    }
    throw Exception(carg, stringify(arg, " is not a list or record"));
}

void
Assoc::exec(Frame& f, Executor& ex) const
{
    ex.push_field(name_.eval(f), definiens_->eval(f), At_Phrase(*syntax_, f));
}
Value
Assoc::eval(Frame& f) const
{
    auto name = name_.eval(f);
    auto elem = definiens_->eval(f);
    Shared<List> pair = List::make({name.to_value(),elem});
    return {pair};
}

Value
Record_Expr::eval(Frame& f) const
{
    auto record = make<DRecord>();
    Record_Executor rex(*record);
    for (auto op : fields_)
        op->exec(f, rex);
    return {record};
}

Shared<Module>
Scope_Executable::eval_module(Frame& f) const
{
    assert(module_slot_ != (slot_t)(-1));
    assert(module_dictionary_ != nullptr);

    Shared<Module> module =
        Module::make(module_dictionary_->size(), module_dictionary_);
    f[module_slot_] = {module};
    Operation::Action_Executor aex;
    for (auto action : actions_)
        action->exec(f, aex);
    return module;
}
void
Scope_Executable::exec(Frame& f) const
{
    if (module_slot_ != (slot_t)(-1)) {
        (void) eval_module(f);
    } else {
        Operation::Action_Executor aex;
        for (auto action : actions_) {
            action->exec(f, aex);
        }
    }
}

void
Boxed_Locative::store(Frame& f, const Operation& expr) const
{
    *reference(f,false) = expr.eval(f);
}

Shared<Locative>
Boxed_Locative::get_field(
    Environ& env,
    Shared<const Phrase> syntax,
    Symbol_Expr selector)
{
    return make<Dot_Locative>(syntax, share(*this), selector);
}

Shared<Locative>
Boxed_Locative::get_element(
    Environ& env,
    Shared<const Phrase> syntax,
    Shared<Operation> index)
{
    return make<Indexed_Locative>(syntax, share(*this), index);
}

Shared<Locative>
Boxed_Locative::lens_get_element(
    Environ& env,
    Shared<const Phrase> syntax,
    Shared<Operation> lens)
{
    return make<Lens_Locative>(syntax, share(*this), lens);
}

Value*
Local_Locative::reference(Frame& f,bool) const
{
    return &f[slot_];
}

Value*
Dot_Locative::reference(Frame& f, bool need_value) const
{
    Value* base = base_->reference(f,true);
    Shared<Record> base_rec = base->to<Record>(At_Phrase(*base_->syntax_, f));
    if (base_rec->use_count > 1) {
        base_rec = base_rec->clone();
        *base = {base_rec};
    }
    Symbol_Ref id = selector_.eval(f);
    return base_rec->ref_field(id, need_value, At_Phrase(*syntax_, f));
}

Value*
Indexed_Locative::reference(Frame& f, bool need_value) const
{
    Value* base = base_->reference(f,true);
    auto ix = index_->eval(f);
    if (auto base_rec = base->maybe<Record>()) {
        if (base_rec->use_count > 1) {
            base_rec = base_rec->clone();
            *base = {base_rec};
        }
        At_Phrase icx(*index_->syntax_, f);
        auto index_list = ix.to<List>(icx);
        index_list->assert_size(1, icx);
        auto key = value_to_symbol(index_list->at(0), icx);
        return base_rec->ref_field(key, need_value, At_Phrase(*syntax_, f));
    }
    Shared<List> base_list = base->to<List>(At_Phrase(*base_->syntax_, f));
    if (base_list->use_count > 1) {
        base_list = base_list->clone();
        *base = {base_list};
    }
    return base_list->ref_element(ix, need_value, At_Phrase(*syntax_, f));
}

Value*
Lens_Locative::reference(Frame& f, bool need_value) const
{
    Value* base = base_->reference(f,true);
    auto ix = lens_->eval(f);
    if (auto base_rec = base->maybe<Record>()) {
        if (base_rec->use_count > 1) {
            base_rec = base_rec->clone();
            *base = {base_rec};
        }
        auto key = value_to_symbol(ix, At_Phrase(*lens_->syntax_, f));
        return base_rec->ref_field(key, need_value, At_Phrase(*syntax_, f));
    }
    Shared<List> base_list = base->to<List>(At_Phrase(*base_->syntax_, f));
    if (base_list->use_count > 1) {
        base_list = base_list->clone();
        *base = {base_list};
    }
    return base_list->ref_lens(ix, need_value, At_Phrase(*syntax_, f));
}

void
Assignment_Action::exec(Frame& f, Executor&) const
{
    locative_->store(f, *expr_);
}

Value
Module_Expr::eval(Frame& f) const
{
    auto module = eval_module(f);
    return {module};
}

Shared<Module>
Enum_Module_Expr::eval_module(Frame& f) const
{
    Shared<Module> module = Module::make(exprs_.size(), dictionary_);
    for (size_t i = 0; i < exprs_.size(); ++i)
        module->at(i) = exprs_[i]->eval(f);
    return module;
}

Shared<Module>
Scoped_Module_Expr::eval_module(Frame& f) const
{
    return executable_.eval_module(f);
}

Value
Block_Op::eval(Frame& f) const
{
    statements_.exec(f);
    return body_->eval(f);
}
void
Block_Op::tail_eval(std::unique_ptr<Frame>& f) const
{
    statements_.exec(*f);
    body_->tail_eval(f);
}
void
Block_Op::exec(Frame& f, Executor& ex) const
{
    statements_.exec(f);
    body_->exec(f, ex);
}

Value
Do_Expr::eval(Frame& f) const
{
    Action_Executor aex;
    actions_->exec(f, aex);
    return body_->eval(f);
}
void
Do_Expr::tail_eval(std::unique_ptr<Frame>& f) const
{
    Action_Executor aex;
    actions_->exec(*f, aex);
    body_->tail_eval(f);
}

void
Compound_Op_Base::exec(Frame& f, Executor& ex) const
{
    for (auto s : *this)
        s->exec(f, ex);
}

void
While_Op::exec(Frame& f, Executor& ex) const
{
    for (;;) {
        Value c = cond_->eval(f);
        bool b = c.to_bool(At_Phrase{*cond_->syntax_, f});
        if (!b) return;
        body_->exec(f, ex);
    }
}

void
For_Op::exec(Frame& f, Executor& ex) const
{
    At_Phrase cx{*list_->syntax_, f};
    At_Index icx{0, cx};
    auto values = list_->eval(f);
    if (auto list = values.maybe<const List>()) {
        for (size_t i = 0; i < list->size(); ++i) {
            icx.index_ = i;
            // TODO: For_Op::exec: use icx in pattern_->exec()
            // doesn't work now because not At_Syntax
            pattern_->exec(f.array_, list->at(i), cx, f);
            if (cond_ && !cond_->eval(f).to_bool(At_Phrase{*cond_->syntax_,f}))
                break;
            body_->exec(f, ex);
        }
    } else if (auto string = values.maybe<const String>()) {
        for (size_t i = 0; i < string->size(); ++i) {
            icx.index_ = i;
            // TODO: For_Op::exec: use icx in pattern_->exec()
            // doesn't work now because not At_Syntax
            pattern_->exec(f.array_, {string->at(i)}, cx, f);
            if (cond_ && !cond_->eval(f).to_bool(At_Phrase{*cond_->syntax_,f}))
                break;
            body_->exec(f, ex);
        }
    } else {
        throw Exception(cx, stringify(values, " is not a list"));
    }
}

Value
Range_Expr::eval(Frame& f) const
{
    const char* errmsg;
    Value firstv = arg1_->eval(f);
    double first = firstv.to_num_or_nan();

    Value lastv = arg2_->eval(f);
    double last = lastv.to_num_or_nan();

    Value stepv;
    double step = 1.0;
    if (arg3_) {
        stepv = arg3_->eval(f);
        step = stepv.to_num_or_nan();
    }
    double delta = round((last - first)/step);
    double countd = delta < 0.0 ? 0.0 : delta + (half_open_ ? 0.0 : 1.0);
    if (step != step || step == 0.0 || std::isinf(step)) {
        errmsg = "bad step value";
        goto error;
    }
    // Note: countd could be infinity. It could be too large to fit in an
    // integer. It could be a float integer too large to increment (for large
    // float i, i==i+1). So we impose a limit on the count.
    if (countd < 1'000'000'000.0) {
        List_Builder lb;
        unsigned count = (unsigned) countd;
        for (unsigned i = 0; i < count; ++i)
            lb.push_back(Value{first + step*i});
        return lb.get_value();
    }

    // Fast path failed (assuming Num arguments).
    // Next check for the reactive case.
    if (first==first && last==last && step==step) {
        // all arguments are Num, therefore not a reactive expression
    }
    else if (is_num(firstv) && is_num(lastv)
             && (stepv.is_missing() || is_num(stepv)))
    {
        // For now, this is almost useless: it's triggered when
        // a reactive range value is a parameter to a shape constructor.
        return {make<Reactive_Expression>(
            SC_Type::Error(), // TODO: should be 'Array 1 Num'
            make<Range_Expr>(
                share(*syntax_),
                to_expr(firstv, *arg1_->syntax_),
                to_expr(lastv, *arg2_->syntax_),
                !arg3_ ? nullptr : to_expr(stepv, *arg3_->syntax_),
                half_open_),
            At_Phrase(*syntax_, f))};
    }
    
    // Report error.
    errmsg = (countd == countd ? "too many elements in range" : "domain error");
error:
    const char* dots = (half_open_ ? " ..< " : " .. ");
    throw Exception(At_Phrase(*syntax_, f),
        arg3_
            ? stringify(firstv,dots,lastv," by ",stepv,": ", errmsg)
            : stringify(firstv,dots,lastv,": ", errmsg));
}

Value
Lambda_Expr::eval(Frame& f) const
{
    auto c = make<Closure>(
        pattern_,
        body_,
        nonlocals_->eval_module(f),
        nslots_);
    c->name_ = name_;
    c->argpos_ = argpos_;
    return Value{c};
}

void
Literal_Segment::generate(Frame&, String_Builder& sb) const
{
    sb << *data_;
}
void
Ident_Segment::generate(Frame& f, String_Builder& sb) const
{
    Value val = expr_->eval(f);
    val.print_string(sb);
}
void
Paren_Segment::generate(Frame& f, String_Builder& sb) const
{
    sb << expr_->eval(f);
}
void
Bracket_Segment::generate(Frame& f, String_Builder& sb) const
{
    At_Phrase cx(*expr_->syntax_, f);
    auto list = expr_->eval(f).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i)
        sb << (char)(*list)[i].to_int(1, 127, At_Index(i,cx));
}
void
Brace_Segment::generate(Frame& f, String_Builder& sb) const
{
    At_Phrase cx(*expr_->syntax_, f);
    auto val = expr_->eval(f);
    if (auto str = val.maybe<String>())
        sb << *str;
    else {
        auto list = val.to<List>(cx);
        for (auto val : *list)
            val.print_string(sb);
    }
}
Value
String_Expr_Base::eval(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return sb.get_value();
}
Symbol_Ref
String_Expr_Base::eval_symbol(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return make_symbol(sb.str());
}

void
Data_Setter::exec(Frame& f, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &f[0];
    else {
        auto mval = f[module_slot_];
        auto m = (Module*)&mval.to_ref_unsafe();
        assert(m->subtype_ == Ref_Value::sty_module);
        slots = &m->at(0);
    }
    Value value = definiens_->eval(f);
    pattern_->exec(slots, value, At_Phrase(*definiens_->syntax_, f), f);
}

void
Function_Setter_Base::exec(Frame& f, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &f[0];
    else {
        auto mval = f[module_slot_];
        auto m = (Module*)&mval.to_ref_unsafe();
        assert(m->subtype_ == Ref_Value::sty_module);
        slots = &m->at(0);
    }
    Shared<Module> nonlocals = nonlocals_->eval_module(f);
    for (auto& e : *this)
        slots[e.slot_] = {make<Closure>(*e.lambda_, *nonlocals)};
}

void
Include_Setter_Base::exec(Frame& f, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &f[0];
    else {
        auto mval = f[module_slot_];
        auto m = (Module*)&mval.to_ref_unsafe();
        assert(m->subtype_ == Ref_Value::sty_module);
        slots = &m->at(0);
    }
    for (auto& e : *this)
        slots[e.slot_] = e.value_;
}

// `val :: pred` is a predicate assertion.
// It aborts if `pred val` is false, returns val if `pred val` is true.
Value
Predicate_Assertion_Expr::eval(Frame& f) const
{
    Value val = arg1_->eval(f);
    Value pred = arg2_->eval(f);
    bool r =
        call_func(pred, val, syntax_, f).to_bool(At_Phrase(*syntax_, f));
    if (r) return val;
    throw Exception(At_Phrase(*syntax_, f), "predicate assertion failed");
}

Value Parametric_Ctor::call(Value arg, Fail fl, Frame& fr) const
{
    At_Phrase acx(arg_part(fr.call_phrase_), fr);
    TRY_DEF(arec, arg.to<const Record>(fl, acx));
    auto drec = make<DRecord>();
    // Merge defl_ with arec; fail if arec contains fields not in defl_;
    // place result in drec.
    defl_->each_field(acx, [&](Symbol_Ref id, Value val) -> void {
        drec->fields_[id] = val;
    });
    for (auto field = arec->iter(); !field->empty(); field->next()) {
        auto id = field->key();
        auto ep = drec->fields_.find(id);
        if (ep != drec->fields_.end())
            ep->second = field->value(acx);
        else {
            FAIL(fl, missing, acx, stringify("bad argument ",id));
        }
    }
    // call parametric record constructor
    TRY_DEF(rval, ctor_->call({drec}, fl, fr));
    auto result = update_drecord(rval, acx); // fault on error
    result->fields_[make_symbol("call")] = {ctor_};
    result->fields_[make_symbol("argument")] = {drec};
    return {result};
}

Value
Parametric_Expr::eval(Frame& f) const
{
    At_Phrase cx(*syntax_, f);
    Value func = ctor_->eval(f);
    auto closure = func.maybe<Closure>();
    if (closure == nullptr)
        throw Exception(cx, "internal error in Parametric_Expr");
    Shared<const Phrase> call_phrase = syntax_; // TODO?
    std::unique_ptr<Frame> f2 {
        Frame::make(closure->nslots_, f.system_, &f, call_phrase, nullptr)
    };
    auto default_arg = record_pattern_default_value(*closure->pattern_,*f2);
    Value res = closure->call({default_arg}, Fail::hard, *f2);
    auto rec = res.to<Record>(cx);
    auto drec = make<DRecord>();
    rec->each_field(cx, [&](Symbol_Ref id, Value val) -> void {
        drec->fields_[id] = val;
    });
    // TODO: The `call` function should return another parametric record.
    drec->fields_[make_symbol("call")] =
        {make<Parametric_Ctor>(closure, default_arg)};
    drec->fields_[make_symbol("argument")] = {default_arg};
    return {drec};
}

Symbol_Ref Symbol_Expr::eval(Frame& f) const
{
    if (id_) return id_->symbol_;
    if (auto str = cast<const String_Expr>(expr_))
        return str->eval_symbol(f);
    auto val = expr_->eval(f);
    return value_to_symbol(val, At_Phrase(*expr_->syntax_, f));
}

} // namespace curv
