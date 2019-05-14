// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/meaning.h>
#include <libcurv/string.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/module.h>
#include <libcurv/context.h>
#include <libcurv/array_op.h>
#include <cmath>
#include <libcurv/math.h>

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
Operation::List_Executor::push_field(Symbol_Ref, Value, const Context& cstmt)
{
    throw Exception(cstmt,
        "illegal statement type: can't add record fields to a list");
}

void
Operation::Record_Executor::push_value(Value, const Context& cstmt)
{
    throw Exception(cstmt,
        "illegal statement type: can't add list elements to a record");
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
    Module& m = (Module&)f[slot_].get_ref_unsafe();
    assert(m.subtype_ == Ref_Value::sty_module);
    return m.at(index_);
}

Value
Nonlocal_Data_Ref::eval(Frame& f) const
{
    return f.nonlocals_->at(slot_);
}

Value
Data_Ref::eval(Frame& f) const
{
    return f[slot_];
}

Value
Dot_Expr::eval(Frame& f) const
{
    Value basev = base_->eval(f);
    Symbol_Ref id = selector_.eval(f);
    return basev.at(id, At_Phrase(*base_->syntax_, f));
}

Value
eval_not(Value x, const Context& cx)
{
    if (x.is_bool())
        return {!x.get_bool_unsafe()};
    if (auto xlist = x.dycast<List>()) {
        Shared<List> result = List::make(xlist->size());
        for (unsigned i = 0; i < xlist->size(); ++i)
            (*result)[i] = eval_not((*xlist)[i], cx);
        return {result};
    }
    throw Exception(cx, stringify("!",x,": domain error"));
}
Value
Not_Expr::eval(Frame& f) const
{
    return eval_not(arg_->eval(f), At_Phrase(*syntax_, f));
}

Value
Positive_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double call(double x) { return +x; }
        Shared<Operation> make_expr(Shared<Operation> x) const
        {
            return make<Positive_Expr>(share(cx.syntax()), std::move(x));
        }
        static auto callstr(Value x) { return stringify("+",x); }
        At_Phrase cx;
        Scalar_Op(const Phrase& ph, Frame& f) : cx(ph,f) {}
    };
    static Unary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(Scalar_Op(*syntax_, f), arg_->eval(f));
}
Value
Negative_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double call(double x) { return -x; }
        Shared<Operation> make_expr(Shared<Operation> x) const
        {
            return make<Negative_Expr>(share(cx.syntax()), std::move(x));
        }
        static auto callstr(Value x) { return stringify("-",x); }
        At_Phrase cx;
        Scalar_Op(const Phrase& ph, Frame& f) : cx(ph,f) {}
    };
    static Unary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(Scalar_Op(*syntax_, f), arg_->eval(f));
}

Value
Add_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return add(a,b, At_Phrase(*syntax_, f));
}
Value
Subtract_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double call(double x, double y) { return x - y; }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            return make<Subtract_Expr>(share(cx.syntax()),
                std::move(x), std::move(y));
        }
        static const char* name() { return "-"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," - ",y);
        }
        At_Phrase cx;
        Scalar_Op(const Phrase& ph, Frame& fr) : cx(ph,fr) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(Scalar_Op(*syntax_, f), a, b);
}
Value
Multiply_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return multiply(a,b, At_Phrase(*syntax_, f));
}
Value
Divide_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double call(double x, double y) { return x / y; }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            return make<Divide_Expr>(share(cx.syntax()),
                std::move(x), std::move(y));
        }
        static const char* name() { return "/"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," / ",y);
        }
        At_Phrase cx;
        Scalar_Op(const Phrase& ph, Frame& fr) : cx(ph,fr) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(Scalar_Op(*syntax_, f), a, b);
}

Value
Or_Expr::eval(Frame& f) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        return {true};
    bool b = arg2_->eval(f).to_bool(At_Phrase(*arg2_->syntax_, f));
    return {b};
}
Value
And_Expr::eval(Frame& f) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (!a)
        return {false};
    bool b = arg2_->eval(f).to_bool(At_Phrase(*arg2_->syntax_, f));
    return {b};
}

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
    auto re = cond.dycast<Reactive_Value>();
    if (re && re->sctype_ == SC_Type::Bool()) {
        Value a2 = arg2_->eval(f);
        Value a3 = arg3_->eval(f);
        return {make<Reactive_Expression>(
            sc_type_join(sc_type_of(a2), sc_type_of(a3)),
            make<If_Else_Op>(
                share(*syntax_),
                make<Constant>(share(*arg1_->syntax_), cond),
                make<Constant>(share(*arg2_->syntax_), a2),
                make<Constant>(share(*arg3_->syntax_), a3)
            ),
            At_Phrase(*syntax_, f))};
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
    auto re = cond.dycast<Reactive_Value>();
    if (re && re->sctype_ == SC_Type::Bool()) {
        Value a2 = arg2_->eval(*f);
        Value a3 = arg3_->eval(*f);
        f->result_ = Value{make<Reactive_Expression>(
            sc_type_join(sc_type_of(a2), sc_type_of(a3)),
            make<If_Else_Op>(
                share(*syntax_),
                make<Constant>(share(*arg1_->syntax_), cond),
                make<Constant>(share(*arg2_->syntax_), a2),
                make<Constant>(share(*arg3_->syntax_), a3)
            ),
            At_Phrase(*syntax_, *f))};
        f->next_op_ = nullptr;
        return;
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
    return {a.equal(b, At_Phrase(*syntax_, f))};
}
Value
Not_Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return {!a.equal(b, At_Phrase(*syntax_, f))};
}
Value
Less_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*syntax_, f),
        stringify(a," < ",b,": domain error"));
}
Value
Greater_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*syntax_, f),
        stringify(a," > ",b,": domain error"));
}
Value
Less_Or_Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*syntax_, f),
        stringify(a," <= ",b,": domain error"));
}
Value
Greater_Or_Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*syntax_, f),
        stringify(a," >= ",b,": domain error"));
}
Value
Power_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double call(double x, double y) { return pow(x,y); }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            return make<Power_Expr>(share(cx.syntax()),
                std::move(x), std::move(y));
        }
        static const char* name() { return "^"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," ^ ",y);
        }
        At_Phrase cx;
        Scalar_Op(const Phrase& ph, Frame& fr) : cx(ph, fr) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(Scalar_Op(*syntax_, f), arg1_->eval(f), arg2_->eval(f));
}

Value
list_at(const List& list, Value index, const Context& cx)
{
    if (auto indices = index.dycast<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto i : *indices)
            (*result)[j++] = list_at(list, i, cx);
        return {result};
    }
    int i = index.to_int(0, (int)(list.size()-1), cx);
    return list[i];
}
#if 0
Value
record_at(const Record& ref, Value index, const Context& cx)
{
    if (auto indices = index.dycast<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto i : *indices)
            (*result)[j++] = record_at(ref, i, cx);
        return {result};
    }
    Symbol_Ref a = index.to<const String>(cx);
    return ref.getfield(a, cx);
}
#endif
Value
string_at(const String& string, Value index, const Context& cx)
{
    // TODO: this code only works for ASCII strings.
    if (auto indices = index.dycast<List>()) {
        String_Builder sb;
        for (auto ival : *indices) {
            int i = ival.to_int(0, (int)(string.size()-1), cx);
            sb << string[i];
        }
        return {sb.get_string()};
    }
    int i = index.to_int(0, (int)(string.size()-1), cx);
    return {make_string(string.data()+i, 1)};
}
Value
value_at_path(Value a, const List& path, Shared<const Phrase> callph, Frame& f)
{
    At_Phrase cx(*arg_part(callph), f);
    At_Index icx(0, cx);
    size_t i = 0;
    for (; i < path.size(); ++i) {
        icx.index_ = i;
        if (auto string = a.dycast<String>()) {
            if (i < path.size()-1)
                goto domain_error;
            return string_at(*string, path[i], icx);
        }
        if (auto list = a.dycast<List>()) {
            if (i < path.size()-1) {
                int j = path[i].to_int(0, (int)(list->size()-1), icx);
                a = list->at(j);
            } else
                a = list_at(*list, path[i], icx);
            continue;
        }
        auto re = a.dycast<Reactive_Value>();
        if (re && re->sctype_.is_vec()) {
            if (i < path.size()-1)
                goto domain_error;
            Value b = path[i];
            if (isnum(b)) {
                return {make<Reactive_Expression>(
                    SC_Type::Num(),
                    make<Call_Expr>(
                        callph,
                        make<Constant>(func_part(callph), a),
                        make<Constant>(arg_part(callph), b)),
                    icx)};
            }
            // TODO: reactive: handle more cases
        }
        goto domain_error;
    }
    return a;
domain_error:
    String_Builder msg;
    msg << a << "[";
    for (size_t j = i; j < path.size(); ++j) {
        if (j > i) msg << ",";
        msg << path[i];
    }
    msg << "]: domain error";
    throw Exception(At_Phrase(*callph, f), msg.str());
}
Value
call_func(Value func, Value arg, Shared<const Phrase> call_phrase, Frame& f)
{
    static Symbol_Ref callkey = "call";
    static Symbol_Ref conskey = "constructor";
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.get_ref_unsafe() );
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
            if (s->hasfield(conskey)) {
                funv = s->getfield(conskey, At_Phrase(*call_phrase, f));
                continue;
            }
            break;
          }
        case Ref_Value::ty_string:
        case Ref_Value::ty_list:
        case Ref_Value::ty_reactive:
          {
            At_Phrase cx(*arg_part(call_phrase), f);
            auto path = arg.to<List>(cx);
            return value_at_path(funv, *path, call_phrase, f);
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
    static Symbol_Ref callkey = "call";
    static Symbol_Ref conskey = "constructor";
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*func_part(call_phrase), *f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.get_ref_unsafe() );
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
            if (s->hasfield(conskey)) {
                funv = s->getfield(conskey, At_Phrase(*call_phrase, *f));
                continue;
            }
            break;
          }
        case Ref_Value::ty_string:
        case Ref_Value::ty_list:
        case Ref_Value::ty_reactive:
          {
            At_Phrase cx(*arg_part(call_phrase), *f);
            auto path = arg.to<List>(cx);
            f->result_ = value_at_path(funv, *path, call_phrase, *f);
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

Shared<List>
List_Expr_Base::eval_list(Frame& f) const
{
    // TODO: if the # of elements generated is known at compile time,
    // then the List could be constructed directly without using a std::vector.
    List_Builder lb;
    List_Executor lex(lb);
    for (size_t i = 0; i < this->size(); ++i)
        (*this)[i]->exec(f, lex);
    return lb.get_list();
}

Value
List_Expr_Base::eval(Frame& f) const
{
    return {eval_list(f)};
}

void
Spread_Op::exec(Frame& f, Executor& ex) const
{
    At_Phrase cstmt(*syntax_, f);
    At_Phrase carg(*arg_->syntax_, f);
    auto arg = arg_->eval(f);
    if (auto list = arg.dycast<const List>()) {
        for (size_t i = 0; i < list->size(); ++i)
            ex.push_value(list->at(i), cstmt);
        return;
    }
    if (auto rec = arg.dycast<const Record>()) {
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
Assignment_Action::exec(Frame& f, Executor&) const
{
    f[slot_] = expr_->eval(f);
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
Preaction_Op::eval(Frame& f) const
{
    Action_Executor aex;
    actions_->exec(f, aex);
    return body_->eval(f);
}
void
Preaction_Op::tail_eval(std::unique_ptr<Frame>& f) const
{
    Action_Executor aex;
    actions_->exec(*f, aex);
    body_->tail_eval(f);
}
void
Preaction_Op::exec(Frame& f, Executor& ex) const
{
    actions_->exec(f, ex);
    body_->exec(f, ex);
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
    auto list = list_->eval(f).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i) {
        icx.index_ = i;
        pattern_->exec(f.array_, list->at(i), icx, f);
        body_->exec(f, ex);
    }
}

Value
Range_Expr::eval(Frame& f) const
{
    List_Builder lb;
    Value firstv = arg1_->eval(f);
    double first = firstv.get_num_or_nan();

    Value lastv = arg2_->eval(f);
    double last = lastv.get_num_or_nan();

    Value stepv;
    double step = 1.0;
    if (arg3_) {
        stepv = arg3_->eval(f);
        step = stepv.get_num_or_nan();
    }

    double delta = round((last - first)/step);
    double countd = delta < 0.0 ? 0.0 : delta + (half_open_ ? 0.0 : 1.0);
    // Note: countd could be infinity. It could be too large to fit in an
    // integer. It could be a float integer too large to increment (for large
    // float i, i==i+1). So we impose a limit on the count.
    if (countd < 1'000'000'000.0) {
        unsigned count = (unsigned) countd;
        for (unsigned i = 0; i < count; ++i)
            lb.push_back(Value{first + step*i});
    } else {
        const char* err =
            (countd == countd ? "too many elements in range" : "domain error");
        const char* dots = (half_open_ ? " ..< " : " .. ");
        throw Exception(At_Phrase(*syntax_, f),
            arg3_
                ? stringify(firstv,dots,lastv," by ",stepv,": ", err)
                : stringify(firstv,dots,lastv,": ", err));
    }
    return {lb.get_list()};
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
    if (auto str = val.dycast<String>())
        sb << *str;
    else
        sb << val;
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
    auto list = expr_->eval(f).to<List>(cx);
    for (auto val : *list) {
        if (auto str = val.dycast<String>())
            sb << *str;
        else
            sb << val;
    }
}
Value
String_Expr_Base::eval(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return {sb.get_string()};
}
Symbol_Ref
String_Expr_Base::eval_symbol(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return {sb.str()};
}

void
Data_Setter::exec(Frame& f, Executor&) const
{
    Value* slots;
    if (module_slot_ == (slot_t)(-1))
        slots = &f[0];
    else {
        auto mval = f[module_slot_];
        auto m = (Module*)&mval.get_ref_unsafe();
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
        auto m = (Module*)&mval.get_ref_unsafe();
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
        auto m = (Module*)&mval.get_ref_unsafe();
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

Value
Parametric_Expr::eval(Frame& f) const
{
    At_Phrase cx(*syntax_, f);
    Value func = ctor_->eval(f);
    auto closure = func.dycast<Closure>();
    if (closure == nullptr)
        throw Exception(cx, "internal error in Parametric_Expr");
    Shared<const Phrase> call_phrase = syntax_; // TODO?
    std::unique_ptr<Frame> f2 {
        Frame::make(closure->nslots_, f.system_, &f, call_phrase, nullptr)
    };
    auto default_arg = record_pattern_default_value(*closure->pattern_,*f2);
    Value res = closure->call({default_arg}, *f2);
    auto rec = res.to<Record>(cx);
    auto drec = make<DRecord>();
    rec->each_field(cx, [&](Symbol_Ref id, Value val) -> void {
        drec->fields_[id] = val;
    });
    // TODO: The `constructor` function should return another parametric record.
    drec->fields_["constructor"] = func;
    drec->fields_["argument"] = {default_arg};
    return {drec};
}

} // namespace curv
