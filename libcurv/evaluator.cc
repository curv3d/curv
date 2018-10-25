// Copyright 2016-2018 Doug Moen
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
Operation::eval(Frame& f) const
{
    throw Exception(At_Phrase(*syntax_, f), "not an expression");
}
void
Operation::exec(Frame& f) const
{
    throw Exception(At_Phrase(*syntax_, f), "not an action");
}
void
Operation::bind(Frame& f, DRecord&) const
{
    throw Exception(At_Phrase(*syntax_, f), "not a binder or action");
}
void
Operation::generate(Frame& f, List_Builder&) const
{
    throw Exception(At_Phrase(*syntax_, f), "not a generator, expression or action");
}

void
Just_Expression::generate(Frame& f, List_Builder& lb) const
{
    lb.push_back(eval(f));
}

void
Just_Action::generate(Frame& f, List_Builder&) const
{
    exec(f);
}
void
Just_Action::bind(Frame& f, DRecord&) const
{
    exec(f);
}

void
Null_Action::exec(Frame&) const
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
    Symbol id = selector_.eval(f);
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
        static double f(double x) { return +x; }
        static Shared<const String> callstr(Value x) {
            return stringify("+",x);
        }
    };
    static Unary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(arg_->eval(f), At_Phrase(*syntax_, f));
}
Value
Negative_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x) { return -x; }
        static Shared<const String> callstr(Value x) {
            return stringify("-",x);
        }
    };
    static Unary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(arg_->eval(f), At_Phrase(*syntax_, f));
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
        static double f(double x, double y) { return x - y; }
        static const char* name() { return "-"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," - ",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*syntax_, f));
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
        static double f(double x, double y) { return x / y; }
        static const char* name() { return "/"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," / ",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*syntax_, f));
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
If_Op::generate(Frame& f, List_Builder& lb) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->generate(f, lb);
}
void
If_Op::bind(Frame& f, DRecord& r) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->bind(f, r);
}
void
If_Op::exec(Frame& f) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->exec(f);
}

Value
If_Else_Op::eval(Frame& f) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        return arg2_->eval(f);
    else
        return arg3_->eval(f);
}
void
If_Else_Op::generate(Frame& f, List_Builder& lb) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->generate(f, lb);
    else
        arg3_->generate(f, lb);
}
void
If_Else_Op::bind(Frame& f, DRecord& r) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->bind(f, r);
    else
        arg3_->bind(f, r);
}
void
If_Else_Op::exec(Frame& f) const
{
    bool a = arg1_->eval(f).to_bool(At_Phrase(*arg1_->syntax_, f));
    if (a)
        arg2_->exec(f);
    else
        arg3_->exec(f);
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
        static double f(double x, double y) { return pow(x,y); }
        static const char* name() { return "^"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," ^ ",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(arg1_->eval(f), arg2_->eval(f), At_Phrase(*syntax_, f));
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
    Symbol a = index.to<const String>(cx);
    return ref.getfield(a, cx);
}
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
    return {String::make(string.data()+i, 1)};
}
Value
value_at_path(Value a, const List& path, const Context& cx)
{
    At_Index icx(0, cx);
    for (size_t i = 0; i < path.size(); ++i) {
        icx.index_ = i;
        if (auto string = a.dycast<String>()) {
            if (i < path.size()-1) {
                throw Exception(icx,
                    "string used with multidimensional indexing (like string[i,j])");
            }
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
        throw Exception(icx, "not list or string");
    }
    return a;
}
Value
Index_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    if (auto list = a.dycast<const List>())
        return list_at(*list, b, At_Phrase(*arg2_->syntax_, f));
    if (auto record = a.dycast<const Record>())
        return record_at(*record, b, At_Phrase(*arg2_->syntax_, f));
    if (auto string = a.dycast<const String>())
        return string_at(*string, b, At_Phrase(*arg2_->syntax_, f));
    throw Exception(At_Phrase(*arg1_->syntax_, f),
        "not a list, record or string");
}
Value
call(Value func, Value arg, const Call_Phrase* call_phrase, Frame& f)
{
    static Symbol callkey = "call";
    Value funv = func;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*call_phrase->function_, f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.get_ref_unsafe() );
        switch (funp.type_) {
        case Ref_Value::ty_function:
          {
            Function* fun = (Function*)&funp;
            std::unique_ptr<Frame> f2 {
                Frame::make(fun->nslots_, f.system_, &f, call_phrase, nullptr)
            };
            return fun->call(arg, *f2);
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
        case Ref_Value::ty_string:
        case Ref_Value::ty_list:
          {
            At_Phrase cx(*call_phrase->arg_, f);
            auto path = arg.to<List>(cx);
            return value_at_path(funv, *path, cx);
          }
        }
        throw Exception(At_Phrase(*call_phrase->function_, f),
            stringify(func,": not a function"));
    }
}
Value
Call_Expr::eval(Frame& f) const
{
    return curv::call(fun_->eval(f), arg_->eval(f), call_phrase(), f);
}

Shared<List>
List_Expr_Base::eval_list(Frame& f) const
{
    // TODO: if the # of elements generated is known at compile time,
    // then the List could be constructed directly without using a std::vector.
    List_Builder lb;
    for (size_t i = 0; i < this->size(); ++i)
        (*this)[i]->generate(f, lb);
    return lb.get_list();
}

Value
List_Expr_Base::eval(Frame& f) const
{
    return {eval_list(f)};
}

void
Spread_Op::generate(Frame& f, List_Builder& lb) const
{
    auto list = arg_->eval(f).to<const List>(At_Phrase(*arg_->syntax_, f));
    for (size_t i = 0; i < list->size(); ++i)
        lb.push_back(list->at(i));
}
void
Spread_Op::bind(Frame& f, DRecord& r) const
{
    At_Phrase cx(*arg_->syntax_, f);
    auto s = arg_->eval(f).to<const Record>(cx);
    for (auto i = s->iter(); !i->empty(); i->next())
        r.fields_[i->key()] = i->value(cx);
}

void
Assoc::bind(Frame& f, DRecord& r) const
{
    Symbol symbol = name_.eval(f);
    r.fields_[symbol] = definiens_->eval(f);
}

Value
Record_Expr::eval(Frame& f) const
{
    auto record = make<DRecord>();
    for (auto op : fields_)
        op->bind(f, *record);
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
    for (auto action : actions_)
        action->exec(f);
    return module;
}
void
Scope_Executable::exec(Frame& f) const
{
    if (module_slot_ != (slot_t)(-1)) {
        (void) eval_module(f);
    } else {
        for (auto action : actions_) {
            action->exec(f);
        }
    }
}

void
Data_Setter::exec(Frame& f) const
{
    f[slot_] = expr_->eval(f);
}

void
Module_Data_Setter::exec(Frame& f) const
{
    Module& m = (Module&)f[slot_].get_ref_unsafe();
    assert(m.subtype_ == Ref_Value::sty_module);
    m.at(index_) = expr_->eval(f);
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
Block_Op::generate(Frame& f, List_Builder& lb) const
{
    statements_.exec(f);
    body_->generate(f, lb);
}
void
Block_Op::bind(Frame& f, DRecord& r) const
{
    statements_.exec(f);
    body_->bind(f, r);
}
void
Block_Op::exec(Frame& f) const
{
    statements_.exec(f);
    body_->exec(f);
}

Value
Preaction_Op::eval(Frame& f) const
{
    actions_->exec(f);
    return body_->eval(f);
}
void
Preaction_Op::generate(Frame& f, List_Builder& lb) const
{
    actions_->exec(f);
    body_->generate(f, lb);
}
void
Preaction_Op::bind(Frame& f, DRecord& r) const
{
    actions_->exec(f);
    body_->bind(f, r);
}
void
Preaction_Op::exec(Frame& f) const
{
    actions_->exec(f);
    body_->exec(f);
}

void
Compound_Op_Base::generate(Frame& f, List_Builder& lb) const
{
    for (auto s : *this)
        s->generate(f, lb);
}
void
Compound_Op_Base::bind(Frame& f, DRecord& r) const
{
    for (auto s : *this)
        s->bind(f, r);
}
void
Compound_Op_Base::exec(Frame& f) const
{
    for (auto s : *this)
        s->exec(f);
}

void
While_Action::exec(Frame& f) const
{
    for (;;) {
        Value c = cond_->eval(f);
        bool b = c.to_bool(At_Phrase{*cond_->syntax_, f});
        if (!b) return;
        body_->exec(f);
    }
}

void
For_Op::generate(Frame& f, List_Builder& lb) const
{
    At_Phrase cx{*list_->syntax_, f};
    At_Index icx{0, cx};
    auto list = list_->eval(f).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i) {
        icx.index_ = i;
        pattern_->exec(f.array_, list->at(i), icx, f);
        body_->generate(f, lb);
    }
}
void
For_Op::bind(Frame& f, DRecord& r) const
{
    At_Phrase cx{*list_->syntax_, f};
    At_Index icx{0, cx};
    auto list = list_->eval(f).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i) {
        icx.index_ = i;
        pattern_->exec(f.array_, list->at(i), icx, f);
        body_->bind(f, r);
    }
}
void
For_Op::exec(Frame& f) const
{
    At_Phrase cx{*list_->syntax_, f};
    At_Index icx{0, cx};
    auto list = list_->eval(f).to<List>(cx);
    for (size_t i = 0; i < list->size(); ++i) {
        icx.index_ = i;
        pattern_->exec(f.array_, list->at(i), icx, f);
        body_->exec(f);
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
        const char* dots = (half_open_ ? "..<" : "..");
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
    Value val = expr_->eval(f);
    if (auto str = val.dycast<String>())
        sb << *str;
    else
        sb << val;
}
Value
String_Expr_Base::eval(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return {sb.get_string()};
}
Symbol
String_Expr_Base::eval_symbol(Frame& f) const
{
    String_Builder sb;
    for (auto seg : *this)
        seg->generate(f, sb);
    return {sb.str()};
}

void
Pattern_Setter::exec(Frame& f) const
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
Function_Setter_Base::exec(Frame& f) const
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
Include_Setter_Base::exec(Frame& f) const
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
        curv::call(pred, val, call_phrase(), f)
        .to_bool(At_Phrase(*syntax_, f));
    if (r) return val;
    throw Exception(At_Phrase(*syntax_, f), "predicate assertion failed");
}

} // namespace curv
