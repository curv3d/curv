// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/tree.h>
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
tail_eval_frame(std::unique_ptr<Frame> fm)
{
    while (fm->next_op_ != nullptr)
        fm->next_op_->tail_eval(fm);
    return fm->result_;
}

Value
Operation::eval(Frame& fm) const
{
    throw Exception(At_Phrase(*syntax_, fm), "not an expression");
}

void
Operation::tail_eval(std::unique_ptr<Frame>& fm) const
{
    fm->result_ = eval(*fm);
    fm->next_op_ = nullptr;
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
Just_Expression::exec(Frame& fm, Executor& ex) const
{
    ex.push_value(eval(fm), At_Phrase(*syntax_,fm));
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
Symbolic_Ref::eval(Frame& fm) const
{
    auto& m = *fm.nonlocals_;
    auto b = m.dictionary_->find(name_);
    assert(b != m.dictionary_->end());
    return m.get(b->second);
}

Value
Module_Data_Ref::eval(Frame& fm) const
{
    Module& m = (Module&)fm[slot_].to_ref_unsafe();
    assert(m.subtype_ == Ref_Value::sty_module);
    return m.at(index_);
}

Value
Nonlocal_Data_Ref::eval(Frame& fm) const
{
    return fm.nonlocals_->at(slot_);
}

Value
Local_Data_Ref::eval(Frame& fm) const
{
    return fm[slot_];
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
Dot_Expr::eval(Frame& fm) const
{
    Value basev = base_->eval(fm);
    Symbol_Ref id = selector_.eval(fm);
    return record_at(basev, id, At_Phrase(*base_->syntax_, fm));
}

#define BOOL_EXPR_EVAL(AND_EXPR, AND, NOT, FALSE) \
Value AND_EXPR::eval(Frame& fm) const \
{ \
    Value av = arg1_->eval(fm); \
    if (av.is_bool()) { \
        /* fast path */ \
        if (NOT av.to_bool_unsafe()) \
            return {FALSE}; \
        Value bv = arg2_->eval(fm); \
        assert_bool(bv, At_Phrase(*arg2_->syntax_, fm)); \
        return bv; \
    } \
    /* slow path, handle case where arg1 is a reactive Bool */ \
    assert_bool(av, At_Phrase(*arg1_->syntax_, fm)); \
    /* TODO: if arg2_->eval aborts, construct Error value and continue. */ \
    Value bv = arg2_->eval(fm); \
    if (bv.is_bool()) { \
        /* The 'return {false}' case is importance for correctness; */ \
        /* see new_core/Reactive "Lazy Boolean Operators" */ \
        bool b = bv.to_bool_unsafe(); \
        if (NOT b) return {FALSE}; else return av; \
    } \
    assert_bool(bv, At_Phrase(*arg2_->syntax_, fm)); \
    return {make<Reactive_Expression>( \
        SC_Type::Bool(), \
        make<AND_EXPR>( \
            share(*syntax_), \
            to_expr(av, *arg1_->syntax_), \
            to_expr(bv, *arg2_->syntax_)), \
        At_Phrase(*syntax_, fm))}; \
} \
void AND_EXPR::print(std::ostream& out) const \
  { out<<"("<<*arg1_<<#AND<<*arg2_<<")"; }
BOOL_EXPR_EVAL(And_Expr, &&, !, false)
BOOL_EXPR_EVAL(Or_Expr, ||, , true)

Value
If_Op::eval(Frame& fm) const
{
    throw Exception{At_Phrase{*syntax_, fm},
        "if: not an expression (missing else clause)"};
}
void
If_Op::exec(Frame& fm, Executor& ex) const
{
    bool a = arg1_->eval(fm).to_bool(At_Phrase(*arg1_->syntax_, fm));
    if (a)
        arg2_->exec(fm, ex);
}

Value
If_Else_Op::eval(Frame& fm) const
{
    Value cond = arg1_->eval(fm);
    At_Phrase cx(*arg1_->syntax_, fm);
    if (cond.is_bool()) {
        if (cond.to_bool(cx))
            return arg2_->eval(fm);
        else
            return arg3_->eval(fm);
    }
    auto re = cond.maybe<Reactive_Value>();
    if (re && re->sctype_.is_bool()) {
        Value a2 = arg2_->eval(fm);
        Value a3 = arg3_->eval(fm);
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
                At_Phrase(*syntax_, fm))};
        }
        throw Exception(At_Phrase(*syntax_, fm),
            stringify("then and else expressions have mismatched types: ",
                t2," and ",t3));
    }
    throw Exception(cx, stringify(cond, " is not a boolean"));
}
void
If_Else_Op::tail_eval(std::unique_ptr<Frame>& fm) const
{
    Value cond = arg1_->eval(*fm);
    At_Phrase cx(*arg1_->syntax_, *fm);
    if (cond.is_bool()) {
        if (cond.to_bool(cx))
            fm->next_op_ = &*arg2_;
        else
            fm->next_op_ = &*arg3_;
        return;
    }
    auto re = cond.maybe<Reactive_Value>();
    if (re && re->sctype_.is_bool()) {
        Value a2 = arg2_->eval(*fm);
        Value a3 = arg3_->eval(*fm);
        SC_Type t2 = sc_type_of(a2);
        SC_Type t3 = sc_type_of(a3);
        if (t2 == t3) {
            fm->result_ = Value{make<Reactive_Expression>(
                t2,
                make<If_Else_Op>(
                    share(*syntax_),
                    to_expr(cond, *arg1_->syntax_),
                    to_expr(a2, *arg2_->syntax_),
                    to_expr(a3, *arg3_->syntax_)),
                At_Phrase(*syntax_, *fm))};
            fm->next_op_ = nullptr;
            return;
        }
        throw Exception(At_Phrase(*syntax_, *fm),
            stringify("then and else expressions have mismatched types: ",
                t2," and ",t3));
    }
    throw Exception(cx, stringify(cond, " is not a boolean"));
}
void
If_Else_Op::exec(Frame& fm, Executor& ex) const
{
    bool a = arg1_->eval(fm).to_bool(At_Phrase(*arg1_->syntax_, fm));
    if (a)
        arg2_->exec(fm, ex);
    else
        arg3_->exec(fm, ex);
}

Value
Equal_Expr::eval(Frame& fm) const
{
    Value a = arg1_->eval(fm);
    Value b = arg2_->eval(fm);
    At_Phrase cx(*syntax_, fm);
    return eqval<Equal_Expr>(a.equal(b, cx), a, b, cx);
}
void Equal_Expr::print(std::ostream& out) const
  { out<<"("<<*arg1_<<"=="<<*arg2_<<")"; }
Value
Not_Equal_Expr::eval(Frame& fm) const
{
    Value a = arg1_->eval(fm);
    Value b = arg2_->eval(fm);
    At_Phrase cx(*syntax_, fm);
    return eqval<Not_Equal_Expr>(!a.equal(b, cx), a, b, cx);
}
void Not_Equal_Expr::print(std::ostream& out) const
  { out<<"("<<*arg1_<<"!="<<*arg2_<<")"; }

Value Index_Expr::eval(Frame& fm) const
{
    return tree_fetch(arg1_->eval(fm), arg2_->eval(fm), At_Phrase(*syntax_, fm));
}
Value Slice_Expr::eval(Frame& fm) const
{
    Value value = arg1_->eval(fm);
    Value slice = arg2_->eval(fm);
    return get_value_at_boxed_slice(value, slice, At_Phrase(*syntax_, fm));
}

Value
call_func(Value func, Value arg, Shared<const Phrase> call_phrase, Frame& fm)
{
    static Symbol_Ref callkey = make_symbol("call");
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), fm),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.to_ref_unsafe() );
        switch (funp.type_) {
        case Ref_Value::ty_function:
          {
            Function* fun = (Function*)&funp;
            std::unique_ptr<Frame> f2 {
                Frame::make(fun->nslots_, fm.sstate_, &fm, call_phrase, nullptr)
            };
            f2->func_ = share(*fun);
            fun->tail_call(arg, f2);
            return tail_eval_frame(move(f2));
          }
        case Ref_Value::ty_record:
          {
            Record* s = (Record*)&funp;
            if (s->hasfield(callkey)) {
                funv = s->getfield(callkey, At_Phrase(*call_phrase, fm));
                continue;
            }
            break;
          }
        case Ref_Value::ty_abstract_list:
        case Ref_Value::ty_reactive:
          {
            fm.sstate_.deprecate(&Source_State::bracket_index_deprecated_, 1,
                At_Phrase(*call_phrase, fm),
                "'array[i]' array indexing is deprecated.\n"
                "Use 'array.[i]' instead.");
            return get_value_at_boxed_slice(funv, arg,
                At_Phrase(*call_phrase, fm));
          }
        }
        throw Exception(At_Phrase(*func_part(call_phrase), fm),
            stringify(func,": not a function"));
    }
}
void
tail_call_func(
    Value func, Value arg,
    Shared<const Phrase> call_phrase, std::unique_ptr<Frame>& fm)
{
    static Symbol_Ref callkey = make_symbol("call");
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), *fm),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.to_ref_unsafe() );
        switch (funp.type_) {
        case Ref_Value::ty_function:
          {
            Function* fun = (Function*)&funp;
            fm = Frame::make(
                fun->nslots_, fm->sstate_, fm->parent_frame_,
                call_phrase, nullptr);
            fm->func_ = share(*fun);
            fun->tail_call(arg, fm);
            return;
          }
        case Ref_Value::ty_record:
          {
            Record* s = (Record*)&funp;
            if (s->hasfield(callkey)) {
                funv = s->getfield(callkey, At_Phrase(*call_phrase, *fm));
                continue;
            }
            break;
          }
        case Ref_Value::ty_abstract_list:
        case Ref_Value::ty_reactive:
          {
            fm->result_ = get_value_at_boxed_slice(funv, arg,
                At_Phrase(*call_phrase, *fm));
            fm->next_op_ = nullptr;
            return;
          }
        }
        throw Exception(At_Phrase(*func_part(call_phrase), *fm),
            stringify(func,": not a function"));
    }
}
Value
Call_Expr::eval(Frame& fm) const
{
    return call_func(func_->eval(fm), arg_->eval(fm), syntax_, fm);
}
void
Call_Expr::tail_eval(std::unique_ptr<Frame>& fm) const
{
    tail_call_func(func_->eval(*fm), arg_->eval(*fm), syntax_, fm);
}

Value
List_Expr_Base::eval(Frame& fm) const
{
    // TODO: If the # of elements generated is known at compile time,
    // then the List could be constructed directly without using a std::vector?
    // The result now depends on the element types, but we could still use
    // the length information to optimize List_Builder.
    List_Builder lb;
    List_Executor lex(lb);
    for (size_t i = 0; i < this->size(); ++i)
        (*this)[i]->exec(fm, lex);
    return lb.get_value();
}
void Paren_List_Expr_Base::exec(Frame& fm, Executor& ex) const
{
    for (size_t i = 0; i < this->size(); ++i)
        this->at(i)->exec(fm, ex);
}

void
Spread_Op::exec(Frame& fm, Executor& ex) const
{
    At_Phrase cstmt(*syntax_, fm);
    At_Phrase carg(*arg_->syntax_, fm);
    auto arg = arg_->eval(fm);
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
Assoc::exec(Frame& fm, Executor& ex) const
{
    ex.push_field(name_.eval(fm), definiens_->eval(fm), At_Phrase(*syntax_, fm));
}
Value
Assoc::eval(Frame& fm) const
{
    auto name = name_.eval(fm);
    auto elem = definiens_->eval(fm);
    Shared<List> pair = List::make({name.to_value(),elem});
    return {pair};
}

Value
Record_Expr::eval(Frame& fm) const
{
    auto record = make<DRecord>();
    Record_Executor rex(*record);
    fields_->exec(fm, rex);
    return {record};
}

Shared<Module>
Scope_Executable::eval_module(Frame& fm) const
{
    assert(module_slot_ != (slot_t)(-1));
    assert(module_dictionary_ != nullptr);

    Shared<Module> module =
        Module::make(module_dictionary_->size(), module_dictionary_);
    fm[module_slot_] = {module};
    Operation::Action_Executor aex;
    for (auto action : actions_)
        action->exec(fm, aex);
    return module;
}
void
Scope_Executable::exec(Frame& fm) const
{
    if (module_slot_ != (slot_t)(-1)) {
        (void) eval_module(fm);
    } else {
        Operation::Action_Executor aex;
        for (auto action : actions_) {
            action->exec(fm, aex);
        }
    }
}

void
Local_Locative::store(Frame& fm, Value val) const
{
    fm[slot_] = val;
}
void Local_Locative::mutate(Frame& fm, std::function<Value(Value)> func) const
{
    fm[slot_] = func(fm[slot_]);
}

void
Indexed_Locative::store(Frame& fm, Value val) const
{
    base_->mutate(fm, [&](Value tree) -> Value {
        auto index = index_->eval(fm);
        return tree_amend(tree, index, val, At_Phrase(*syntax_, fm));
    });
}
void Indexed_Locative::mutate(Frame& fm, std::function<Value(Value)> func) const
{
    base_->mutate(fm, [&](Value tree) -> Value {
        At_Phrase cx(*syntax_, fm);
        Value index = index_->eval(fm);
        Value elems = tree_fetch(tree, index, cx);
        elems = func(elems);
        return tree_amend(tree, index, elems, cx);
    });
}

void
Assignment_Action::exec(Frame& fm, Executor&) const
{
    locative_->store(fm, expr_->eval(fm));
}
void
Mutate_Action::exec(Frame& fm, Executor&) const
{
    //throw Exception(At_Phrase(*syntax_, fm), "a!b not implemented");
    locative_->mutate(fm, [&](Value val) -> Value {
        for (auto& tx : transformers_) {
            auto func = tx.func_expr_->eval(fm);
            val = call_func(func, val, tx.call_phrase_, fm);
        }
        return val;
    });
}

Value
Module_Expr::eval(Frame& fm) const
{
    auto module = eval_module(fm);
    return {module};
}

Shared<Module>
Enum_Module_Expr::eval_module(Frame& fm) const
{
    Shared<Module> module = Module::make(exprs_.size(), dictionary_);
    for (size_t i = 0; i < exprs_.size(); ++i)
        module->at(i) = exprs_[i]->eval(fm);
    return module;
}

Shared<Module>
Scoped_Module_Expr::eval_module(Frame& fm) const
{
    return executable_.eval_module(fm);
}

Value
Block_Op::eval(Frame& fm) const
{
    statements_.exec(fm);
    return body_->eval(fm);
}
void
Block_Op::tail_eval(std::unique_ptr<Frame>& fm) const
{
    statements_.exec(*fm);
    body_->tail_eval(fm);
}
void
Block_Op::exec(Frame& fm, Executor& ex) const
{
    statements_.exec(fm);
    body_->exec(fm, ex);
}

Value
Do_Expr::eval(Frame& fm) const
{
    Action_Executor aex;
    actions_->exec(fm, aex);
    return body_->eval(fm);
}
void
Do_Expr::tail_eval(std::unique_ptr<Frame>& fm) const
{
    Action_Executor aex;
    actions_->exec(*fm, aex);
    body_->tail_eval(fm);
}

void
Compound_Op_Base::exec(Frame& fm, Executor& ex) const
{
    for (auto s : *this)
        s->exec(fm, ex);
}

void
While_Op::exec(Frame& fm, Executor& ex) const
{
    for (;;) {
        Value c = cond_->eval(fm);
        bool b = c.to_bool(At_Phrase{*cond_->syntax_, fm});
        if (!b) return;
        body_->exec(fm, ex);
    }
}

void
For_Op::exec(Frame& fm, Executor& ex) const
{
    At_Phrase cx{*list_->syntax_, fm};
    At_Index icx{0, cx};
    auto values = list_->eval(fm);
    if (auto list = values.maybe<const List>()) {
        for (size_t i = 0; i < list->size(); ++i) {
            icx.index_ = i;
            // TODO: For_Op::exec: use icx in pattern_->exec()
            // doesn't work now because not At_Syntax
            pattern_->exec(fm.array_, list->at(i), cx, fm);
            if (cond_ && cond_->eval(fm).to_bool(At_Phrase{*cond_->syntax_,fm}))
                break;
            body_->exec(fm, ex);
        }
    } else if (auto string = values.maybe<const String>()) {
        for (size_t i = 0; i < string->size(); ++i) {
            icx.index_ = i;
            // TODO: For_Op::exec: use icx in pattern_->exec()
            // doesn't work now because not At_Syntax
            pattern_->exec(fm.array_, {string->at(i)}, cx, fm);
            if (cond_ && cond_->eval(fm).to_bool(At_Phrase{*cond_->syntax_,fm}))
                break;
            body_->exec(fm, ex);
        }
    } else {
        throw Exception(cx, stringify(values, " is not a list"));
    }
}

Value
Range_Expr::eval(Frame& fm) const
{
    const char* errmsg;
    Value firstv = arg1_->eval(fm);
    double first = firstv.to_num_or_nan();

    Value lastv = arg2_->eval(fm);
    double last = lastv.to_num_or_nan();

    Value stepv;
    double step = 1.0;
    if (arg3_) {
        stepv = arg3_->eval(fm);
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
            At_Phrase(*syntax_, fm))};
    }
    
    // Report error.
    errmsg = (countd == countd ? "too many elements in range" : "domain error");
error:
    const char* dots = (half_open_ ? " ..< " : " .. ");
    throw Exception(At_Phrase(*syntax_, fm),
        arg3_
            ? stringify(firstv,dots,lastv," by ",stepv,": ", errmsg)
            : stringify(firstv,dots,lastv,": ", errmsg));
}

Value
Lambda_Expr::eval(Frame& fm) const
{
    auto c = make<Closure>(
        pattern_,
        body_,
        nonlocals_->eval_module(fm),
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
Ident_Segment::generate(Frame& fm, String_Builder& sb) const
{
    Value val = expr_->eval(fm);
    val.print_string(sb);
}
void
Paren_Segment::generate(Frame& fm, String_Builder& sb) const
{
    sb << expr_->eval(fm);
}
void
Bracket_Segment::generate(Frame& fm, String_Builder& sb) const
{
    At_Phrase cx(*expr_->syntax_, fm);
    auto list = expr_->eval(fm).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i)
        sb << (char)(*list)[i].to_int(1, 127, At_Index(i,cx));
}
void
Brace_Segment::generate(Frame& fm, String_Builder& sb) const
{
    At_Phrase cx(*expr_->syntax_, fm);
    auto val = expr_->eval(fm);
    if (auto str = val.maybe<String>())
        sb << *str;
    else {
        auto list = val.to<List>(cx);
        for (auto val : *list)
            val.print_string(sb);
    }
}
Value
String_Expr_Base::eval(Frame& fm) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(fm, sb);
    return sb.get_value();
}
Symbol_Ref
String_Expr_Base::eval_symbol(Frame& fm) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(fm, sb);
    return make_symbol(sb.str());
}

void
Data_Setter::exec(Frame& fm, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &fm[0];
    else {
        auto mval = fm[module_slot_];
        auto m = (Module*)&mval.to_ref_unsafe();
        assert(m->subtype_ == Ref_Value::sty_module);
        slots = &m->at(0);
    }
    Value value = definiens_->eval(fm);
    pattern_->exec(slots, value, At_Phrase(*definiens_->syntax_, fm), fm);
}

void
Function_Setter_Base::exec(Frame& fm, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &fm[0];
    else {
        auto mval = fm[module_slot_];
        auto m = (Module*)&mval.to_ref_unsafe();
        assert(m->subtype_ == Ref_Value::sty_module);
        slots = &m->at(0);
    }
    Shared<Module> nonlocals = nonlocals_->eval_module(fm);
    for (auto& e : *this)
        slots[e.slot_] = {make<Closure>(*e.lambda_, *nonlocals)};
}

void
Include_Setter_Base::exec(Frame& fm, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &fm[0];
    else {
        auto mval = fm[module_slot_];
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
Predicate_Assertion_Expr::eval(Frame& fm) const
{
    Value val = arg1_->eval(fm);
    Value pred = arg2_->eval(fm);
    bool r =
        call_func(pred, val, syntax_, fm).to_bool(At_Phrase(*syntax_, fm));
    if (r) return val;
    throw Exception(At_Phrase(*syntax_, fm), "predicate assertion failed");
}

Value Parametric_Ctor::call(Value arg, Fail fl, Frame& fm) const
{
    At_Phrase acx(arg_part(fm.call_phrase_), fm);
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
    TRY_DEF(rval, ctor_->call({drec}, fl, fm));
    auto result = update_drecord(rval, acx); // fault on error
    result->fields_[make_symbol("call")] = {ctor_};
    result->fields_[make_symbol("argument")] = {drec};
    return {result};
}

Value
Parametric_Expr::eval(Frame& fm) const
{
    At_Phrase cx(*syntax_, fm);
    At_Phrase cxbody(*ctor_->body_->syntax_, fm);
    Value func = ctor_->eval(fm);
    auto closure = func.maybe<Closure>();
    if (closure == nullptr)
        throw Exception(cx, "internal error in Parametric_Expr");
    Shared<const Phrase> call_phrase = syntax_; // TODO?
    std::unique_ptr<Frame> f2 {
        Frame::make(closure->nslots_, fm.sstate_, &fm, call_phrase, nullptr)
    };
    auto default_arg = record_pattern_default_value(*closure->pattern_,*f2);
    Value res = closure->call({default_arg}, Fail::hard, *f2);
    auto rec = res.to<Record>(cxbody);
    auto drec = make<DRecord>();
    rec->each_field(cxbody, [&](Symbol_Ref id, Value val) -> void {
        drec->fields_[id] = val;
    });
    // TODO: The `call` function should return another parametric record.
    drec->fields_[make_symbol("call")] =
        {make<Parametric_Ctor>(closure, default_arg)};
    drec->fields_[make_symbol("argument")] = {default_arg};
    return {drec};
}

Symbol_Ref Symbol_Expr::eval(Frame& fm) const
{
    if (id_) return id_->symbol_;
    if (auto str = cast<const String_Expr>(expr_))
        return str->eval_symbol(fm);
    auto val = expr_->eval(fm);
    return value_to_symbol(val, At_Phrase(*expr_->syntax_, fm));
}

Value TPath_Expr::eval(Frame& fm) const
{
    std::vector<Value> ivals;
    for (auto i : indexes_)
        ivals.push_back(i->eval(fm));
    return make_tpath(&ivals[0], &ivals[ivals.size()]);
}
Value TSlice_Expr::eval(Frame& fm) const
{
    Value ival = indexes_->eval(fm);
    auto ilist = ival.to<List>(At_Phrase(*indexes_->syntax_, fm));
    return make_tslice(ilist->begin(), ilist->end());
}

} // namespace curv
