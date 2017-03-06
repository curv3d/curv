// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/arg.h>
#include <curv/meaning.h>
#include <curv/eval.h>
#include <curv/string.h>
#include <curv/exception.h>
#include <curv/function.h>
#include <curv/list.h>
#include <curv/record.h>
#include <curv/module.h>
#include <curv/thunk.h>
#include <curv/context.h>
#include <curv/array_op.h>
#include <cmath>

namespace curv {

Value
Operation::eval(Frame& f) const
{
    throw Exception(At_Phrase(*source_, &f), "not an expression");
}
void
Operation::exec(Frame& f) const
{
    throw Exception(At_Phrase(*source_, &f), "not an action");
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
Null_Action::exec(Frame&) const
{
}

Value
Constant::eval(Frame&) const
{
    return value_;
}

Value
Module_Ref::eval(Frame& f) const
{
    // Modules are lazily evaluated. From here, I need access to both the module
    // field Value (if computed), and the original field expression (otherwise).
    // Implementation options:
    // 1. Initialize f.module_.fields with the expression values, which are
    //    effectively lazy evaluation thunks. Replace with Values on demand.
    // 2. Instead of making Expression a subclass of Ref_Value, introduce
    //    a Lazy_Thunk value class. It's a variation of Function. It contains
    //    the # of local frame slots needed to evaluate `letrec` exprs in the
    //    definiens, plus the Expression.
    // 3. Store a reference to the Expression in Module_Ref.
    // 4. Store a reference to the Module_Expr in Frame.
    // Choice: #2.

    return force_ref((*f.nonlocal)[slot_], *source_, f);
}

Value
Nonlocal_Ref::eval(Frame& f) const
{
    return (*f.nonlocal)[slot_];
}

Value
Letrec_Ref::eval(Frame& f) const
{
    // Let bindings are represented as slots in the frame, and are lazily
    // evaluated. The slots are initialized with thunks. On first reference,
    // the thunk is evaluated and the slot is updated with the resulting value.
    // Recursion is detected and causes an error.

    return force_ref(f[slot_], *source_, f);
}

Value
Let_Ref::eval(Frame& f) const
{
    return f[slot_];
}

Value
Arg_Ref::eval(Frame& f) const
{
    return f[slot_];
}

Value
Local_Function_Ref::eval(Frame& f) const
{
    return {make<Closure>(
        (Lambda&) f[lambda_slot_].get_ref_unsafe(),
        (List&) f[env_slot_].get_ref_unsafe())};
}

Value
Nonlocal_Function_Ref::eval(Frame& f) const
{
    return {make<Closure>(
        (Lambda&) (*f.nonlocal)[lambda_slot_].get_ref_unsafe(),
        *f.nonlocal)};
}

Value
Call_Expr::eval(Frame& f) const
{
    Value funv = fun_->eval(f);
    if (!funv.is_ref())
        throw Exception(At_Phrase(*fun_->source_, &f),
            stringify(funv,": not a function"));
    Ref_Value& funp( funv.get_ref_unsafe() );
    if (funp.type_ == Ref_Value::ty_function) {
        Function* fun = (Function*)&funp;
        if (args_.size() != fun->nargs_) {
            throw Exception(At_Phrase(*call_phrase()->args_, &f),
                "wrong number of arguments");
        }
        std::unique_ptr<Frame> f2 {
            Frame::make(fun->nslots_, f.system, &f, call_phrase(), nullptr)
        };
        for (size_t i = 0; i < args_.size(); ++i)
            (*f2)[i] = args_[i]->eval(f);
        return fun->call(*f2);
    } else {
        throw Exception(At_Phrase(*fun_->source_, &f),
            stringify(funv,": not a function"));
    }
}

Value
Dot_Expr::eval(Frame& f) const
{
    Value basev = base_->eval(f);
    if (basev.is_ref()) {
        Ref_Value& basep( basev.get_ref_unsafe() );
        Value val = basep.getfield(id_);
        if (val != missing)
            return val;
    }
    throw Exception(At_Phrase(*base_->source_, &f),
        stringify(".",id_,": not defined"));
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
    return eval_not(arg_->eval(f), At_Phrase(*source_, &f));
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
    return array_op.op(arg_->eval(f), At_Phrase(*source_, &f));
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
    return array_op.op(arg_->eval(f), At_Phrase(*source_, &f));
}

Value
Add_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x, double y) { return x + y; }
        static const char* name() { return "+"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"+",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;

    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*source_, &f));
}
Value
Subtract_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x, double y) { return x - y; }
        static const char* name() { return "-"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"-",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*source_, &f));
}
Value
Multiply_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x, double y) { return x * y; }
        static const char* name() { return "*"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"*",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*source_, &f));
}
Value
Divide_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x, double y) { return x / y; }
        static const char* name() { return "/"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"/",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return array_op.op(a,b, At_Phrase(*source_, &f));
}

Value
Block_Op_Base::eval(Frame& f) const
{
    for (size_t i = 0; i < size()-1; ++i)
        at(i)->exec(f);
    return back()->eval(f);
}
void
Block_Op_Base::generate(Frame& f, List_Builder& lb) const
{
    for (size_t i = 0; i < size()-1; ++i)
        at(i)->exec(f);
    back()->generate(f, lb);
}
void
Block_Op_Base::exec(Frame& f) const
{
    for (size_t i = 0; i < size(); ++i)
        at(i)->exec(f);
}

Value
Or_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        return a;
    if (a == Value{false}) {
        Value b = arg2_->eval(f);
        if (b.is_bool())
            return b;
        throw Exception(At_Phrase(*arg2_->source_, &f), "not a boolean value");
    }
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
Value
And_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    if (a == Value{false})
        return a;
    if (a == Value{true}) {
        Value b = arg2_->eval(f);
        if (b.is_bool())
            return b;
        throw Exception(At_Phrase(*arg2_->source_, &f), "not a boolean value");
    }
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}

Value
If_Op::eval(Frame& f) const
{
    throw Exception{At_Phrase{*source_, &f},
        "if: not an expression (missing else clause)"};
}
void
If_Op::generate(Frame& f, List_Builder& lb) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->generate(f, lb);
    else if (a == Value{false})
        return;
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
void
If_Op::exec(Frame& f) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->exec(f);
    else if (a == Value{false})
        return;
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}

Value
If_Else_Op::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        return arg2_->eval(f);
    if (a == Value{false})
        return arg3_->eval(f);
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
void
If_Else_Op::generate(Frame& f, List_Builder& lb) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->generate(f, lb);
    else if (a == Value{false})
        arg3_->generate(f, lb);
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
void
If_Else_Op::exec(Frame& f) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->exec(f);
    else if (a == Value{false})
        arg3_->exec(f);
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}

Value
Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return {a == b};
}
Value
Not_Equal_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return {a != b};
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
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,"<",b,": domain error"));
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
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,">",b,": domain error"));
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
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,"<=",b,": domain error"));
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
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,">=",b,": domain error"));
}
Value
Power_Expr::eval(Frame& f) const
{
    struct Scalar_Op {
        static double f(double x, double y) { return pow(x,y); }
        static const char* name() { return "^"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"^",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(arg1_->eval(f), arg2_->eval(f), At_Phrase(*source_, &f));
}

Value
eval_at(const List& list, Value index, const Context& cx)
{
    if (auto indices = index.dycast<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto i : *indices)
            (*result)[j++] = eval_at(list, i, cx);
        return {result};
    }
    int i = arg_to_int(index, 0, (int)(list.size()-1), cx);
    return list[i];
}
Value
At_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    auto& list {arg_to_list(a, At_Phrase(*arg1_->source_, &f))};
    return eval_at(list, b, At_Phrase(*arg2_->source_, &f));
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
Spread_Gen::generate(Frame& f, List_Builder& lb) const
{
    auto list = arg_->eval(f).to<const List>(At_Phrase(*source_, &f));
    for (size_t i = 0; i < list->size(); ++i)
        lb.push_back(list->at(i));
}

Value
Record_Expr::eval(Frame& f) const
{
    auto record = make<Record>();
    for (auto i : fields_)
        record->fields_[i.first] = i.second->eval(f);
    return {record};
}

Value
Module_Expr::eval(Frame& f) const
{
    auto module = eval_module(f.system, &f);
    return {module};
}

Shared<Module>
Module_Expr::eval_module(System& sys, Frame* f) const
{
    auto module = make<Module>();
    module->dictionary_ = dictionary_;
    module->slots_ = List::make_copy(slots_->begin(), slots_->size());
    std::unique_ptr<Frame> frame
        {Frame::make(frame_nslots_, sys, f, nullptr, &*module->slots_)};
    for (Value& s : *module->slots_)
        force(s, *frame);
    module->elements_ = elements_->eval_list(*frame);
    return module;
}

Value
Let_Op::eval(Frame& f) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < exprs_.size(); ++i)
        slots[i] = exprs_[i]->eval(f);
    return body_->eval(f);
}
void
Let_Op::generate(Frame& f, List_Builder& lb) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < exprs_.size(); ++i)
        slots[i] = exprs_[i]->eval(f);
    body_->generate(f, lb);
}
void
Let_Op::exec(Frame& f) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < exprs_.size(); ++i)
        slots[i] = exprs_[i]->eval(f);
    body_->exec(f);
}

Value
Letrec_Op::eval(Frame& f) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < values_.size(); ++i)
        slots[i] = values_[i];
    return body_->eval(f);
}
void
Letrec_Op::generate(Frame& f, List_Builder& lb) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < values_.size(); ++i)
        slots[i] = values_[i];
    body_->generate(f, lb);
}
void
Letrec_Op::exec(Frame& f) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < values_.size(); ++i)
        slots[i] = values_[i];
    body_->exec(f);
}

void
For_Op::generate(Frame& f, List_Builder& lb) const
{
    Value listval = list_->eval(f);
    List& list = arg_to_list(listval, At_Phrase{*list_->source_, &f});
    for (size_t i = 0; i < list.size(); ++i) {
        f[slot_] = list[i];
        body_->generate(f, lb);
    }
}
void
For_Op::exec(Frame& f) const
{
    Value listval = list_->eval(f);
    List& list = arg_to_list(listval, At_Phrase{*list_->source_, &f});
    for (size_t i = 0; i < list.size(); ++i) {
        f[slot_] = list[i];
        body_->exec(f);
    }
}

Value
Range_Expr::eval(Frame& f) const
{
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
        auto list = List::make(count);
        for (unsigned i = 0; i < count; ++i)
            list->at(i) = Value{first + step*i};
        return {std::move(list)};
    } else {
        const char* err =
            (countd == countd ? "too many elements in range" : "domain error");
        const char* dots = (half_open_ ? "..<" : "..");
        throw Exception(At_Phrase(*source_, &f),
            arg3_
                ? stringify(firstv,dots,lastv," by ",stepv,": ", err)
                : stringify(firstv,dots,lastv,": ", err));
    }
}

Value
Lambda_Expr::eval(Frame& f) const
{
    return Value{make<Closure>(
        body_,
        nonlocals_->eval_list(f),
        nargs_,
        nslots_)};
}

} // namespace curv
