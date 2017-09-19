// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/arg.h>
#include <curv/meaning.h>
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
#include <curv/math.h>

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
Operation::bind(Frame& f, Record&) const
{
    throw Exception(At_Phrase(*source_, &f), "not a binder or action");
}
void
Operation::generate(Frame& f, List_Builder&) const
{
    throw Exception(At_Phrase(*source_, &f), "not a generator, expression or action");
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
Just_Action::bind(Frame& f, Record&) const
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
Nonlocal_Lazy_Ref::eval(Frame& f) const
{
    return force_ref(*f.nonlocal, slot_, *source_, f);
}

Value
Indirect_Lazy_Ref::eval(Frame& f) const
{
    List& list = (List&)f[slot_].get_ref_unsafe();
    assert(list.type_ == Ref_Value::ty_list);
    return force_ref(list, index_, *source_, f);
}

Value
Indirect_Strict_Ref::eval(Frame& f) const
{
    List& list = (List&)f[slot_].get_ref_unsafe();
    assert(list.type_ == Ref_Value::ty_list);
    return list[index_];
}

Value
Nonlocal_Strict_Ref::eval(Frame& f) const
{
    return (*f.nonlocal)[slot_];
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
Nonlocal_Function_Ref::eval(Frame& f) const
{
    return {make<Closure>(
        (Lambda&) (*f.nonlocal)[lambda_slot_].get_ref_unsafe(),
        *f.nonlocal)};
}

Value
Indirect_Function_Ref::eval(Frame& f) const
{
    List& list = (List&)f[slot_].get_ref_unsafe();

    // TODO: Kludge. We are seeing <thunk> values in the GL compiler.
    assert(list.type_ == Ref_Value::ty_list);
    for (size_t i = 0; i < list.size(); ++i)
        force(list, i, f);

    Lambda& lambda = (Lambda&) list[index_].get_ref_unsafe();
    assert(lambda.type_ == Ref_Value::ty_lambda);

    return {make<Closure>(lambda, list)};
}

Value
Dot_Expr::eval(Frame& f) const
{
    Value basev = base_->eval(f);
    return basev.at(id_, At_Phrase(*base_->source_, &f));
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
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return add(a,b, At_Phrase(*source_, &f));
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
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    return multiply(a,b, At_Phrase(*source_, &f));
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
If_Op::bind(Frame& f, Record& r) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->bind(f, r);
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
If_Else_Op::bind(Frame& f, Record& r) const
{
    Value a = arg1_->eval(f);
    if (a == Value{true})
        arg2_->bind(f, r);
    else if (a == Value{false})
        arg3_->bind(f, r);
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
list_at(const List& list, Value index, const Context& cx)
{
    if (auto indices = index.dycast<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto i : *indices)
            (*result)[j++] = list_at(list, i, cx);
        return {result};
    }
    int i = arg_to_int(index, 0, (int)(list.size()-1), cx);
    return list[i];
}
Value
struct_at(const Structure& ref, Value index, const Context& cx)
{
    if (auto indices = index.dycast<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto i : *indices)
            (*result)[j++] = struct_at(ref, i, cx);
        return {result};
    }
    Atom a = index.to<const String>(cx);
    return ref.getfield(a, cx);
}
Value
string_at(const String& string, Value index, const Context& cx)
{
    // TODO: this code only works for ASCII strings.
    if (auto indices = index.dycast<List>()) {
        String_Builder sb;
        for (auto ival : *indices) {
            int i = arg_to_int(ival, 0, (int)(string.size()-1), cx);
            sb << string[i];
        }
        return {sb.get_string()};
    }
    int i = arg_to_int(index, 0, (int)(string.size()-1), cx);
    return {String::make(string.data()+i, 1)};
}
Value
Index_Expr::eval(Frame& f) const
{
    Value a = arg1_->eval(f);
    Value b = arg2_->eval(f);
    if (auto list = a.dycast<const List>())
        return list_at(*list, b, At_Phrase(*arg2_->source_, &f));
    if (auto structure = a.dycast<const Structure>())
        return struct_at(*structure, b, At_Phrase(*arg2_->source_, &f));
    if (auto string = a.dycast<const String>())
        return string_at(*string, b, At_Phrase(*arg2_->source_, &f));
    throw Exception(At_Phrase(*arg1_->source_, &f),
        "not a list, structure or string");
}
Value
Call_Expr::eval(Frame& f) const
{
    static Atom mapkey = "map";
    Value val = fun_->eval(f);
    Value funv = val;
    for (;;) {
        if (!funv.is_ref())
            throw Exception(At_Phrase(*fun_->source_, &f),
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.get_ref_unsafe() );
        if (funp.type_ == Ref_Value::ty_function) {
            Function* fun = (Function*)&funp;
            std::unique_ptr<Frame> f2 {
                Frame::make(fun->nslots_, f.system, &f, call_phrase(), nullptr)
            };
            return fun->call(arg_->eval(f), *f2);
        }
        if (auto s = dynamic_cast<Structure*>(&funp)) {
            if (s->hasfield(mapkey)) {
                funv = s->getfield(mapkey, {});
                continue;
            }
            break;
        }
        At_Phrase cx(*arg_->source_, &f);
        auto path = arg_->eval(f).to<List>(cx);
        // TODO: support path with count > 1.
        path->assert_size(1, cx);
        At_Index cx0(0, cx);
        if (auto list = dynamic_cast<const List*>(&funp))
            return list_at(*list, path->at(0), cx0);
        if (auto string = dynamic_cast<const String*>(&funp))
            return string_at(*string, path->at(0), cx0);
        break;
    }
    throw Exception(At_Phrase(*fun_->source_, &f),
        stringify(val,": not a function"));
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
    auto list = arg_->eval(f).to<const List>(At_Phrase(*source_, &f));
    for (size_t i = 0; i < list->size(); ++i)
        lb.push_back(list->at(i));
}
void
Spread_Op::bind(Frame& f, Record& r) const
{
    auto s = arg_->eval(f).to<const Structure>(At_Phrase(*source_, &f));
    s->putfields(r.fields_);
}

void
Assoc::bind(Frame& f, Record& r) const
{
    r.fields_[name_->atom_] = definiens_->eval(f);
}

Value
Record_Expr::eval(Frame& f) const
{
    auto record = make<Record>();
    for (auto op : fields_)
        op->bind(f, *record);
    return {record};
}

void
Statements::exec(Frame& f) const
{
    if (slot_ != (slot_t)(-1)) {
        slot_t ndefns = defn_values_->size();
        slot_t nvalues = ndefns + nonlocal_exprs_.size();
        Shared<List> values = List::make(nvalues);
        for (slot_t i = 0; i < ndefns; ++i)
            values->at(i) = defn_values_->at(i);
        for (slot_t i = ndefns; i < nvalues; ++i)
            values->at(i) = nonlocal_exprs_[i - ndefns]->eval(f);
        f[slot_] = {values};
    }
    for (auto action : actions_) {
        action->exec(f);
    }
}

Shared<List>
Statements::eval(Frame& f) const
{
    assert(slot_ != (slot_t)(-1));
    slot_t ndefns = defn_values_->size();
    slot_t nvalues = ndefns + nonlocal_exprs_.size();
    Shared<List> values = List::make(nvalues);
    for (slot_t i = 0; i < ndefns; ++i)
        values->at(i) = defn_values_->at(i);
    for (slot_t i = ndefns; i < nvalues; ++i)
        values->at(i) = nonlocal_exprs_[i - ndefns]->eval(f);
    f[slot_] = {values};
    for (auto action : actions_) {
        action->exec(f);
    }
#if 1
    // TODO: Get rid of this and make modules fully lazy.
    // Requires code to force module fields wherever they are accessed, eg
    // using dot notation, and also in the GL compiler.
    for (slot_t i = 0; i < ndefns; ++i)
        force(*values, i, f);
#endif
    return values;
}

void
Let_Assign::exec(Frame& f) const
{
    f[slot_] = expr_->eval(f);
}

void
Indirect_Assign::exec(Frame& f) const
{
    List& list = (List&)f[slot_].get_ref_unsafe();
    assert(list.type_ == Ref_Value::ty_list);
    list[index_] = expr_->eval(f);
}

Value
Module_Expr::eval(Frame& f) const
{
    auto module = eval_module(f);
    return {module};
}

Shared<Module>
Module_Expr::eval_module(Frame& f) const
{
    return make<Module>(dictionary_, statements_.eval(f));
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
Block_Op::bind(Frame& f, Record& r) const
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

void
While_Action::exec(Frame& f) const
{
    for (;;) {
        Value c = cond_->eval(f);
        bool b = c.to_bool(At_Phrase{*cond_->source_, &f});
        if (!b) return;
        body_->exec(f);
    }
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
For_Op::bind(Frame& f, Record& r) const
{
    Value listval = list_->eval(f);
    List& list = arg_to_list(listval, At_Phrase{*list_->source_, &f});
    for (size_t i = 0; i < list.size(); ++i) {
        f[slot_] = list[i];
        body_->bind(f, r);
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
        throw Exception(At_Phrase(*source_, &f),
            arg3_
                ? stringify(firstv,dots,lastv," by ",stepv,": ", err)
                : stringify(firstv,dots,lastv,": ", err));
    }
    return {lb.get_list()};
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
