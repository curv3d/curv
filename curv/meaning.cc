// Copyright Doug Moen 2016.
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
#include <cmath>

using namespace curv;

void
curv::Expression::generate(Frame& f, List_Builder& lb) const
{
    lb.push_back(eval(f));
}

Value
curv::Constant::eval(Frame&) const
{
    return value_;
}

Value
curv::Module_Ref::eval(Frame& f) const
{
    // Modules are lazily evaluated. From here, I need access to both the module
    // field Value (if computed), and the original field Expression (otherwise).
    // Implementation options:
    // 1. Initialize f.module_.fields with the Expression values, which are
    //    effectively lazy evaluation thunks. Replace with Values on demand.
    // 2. Instead of making Expression a subclass of Ref_Value, introduce
    //    a Lazy_Thunk value class. It's a variation of Function. It contains
    //    the # of local frame slots needed to evaluate `let` exprs in the
    //    definiens, plus the Expression.
    // 3. Store a reference to the Expression in Module_Ref.
    // 4. Store a reference to the Module_Expr in Frame.
    // Choice: #2.

    return force_ref((*f.nonlocal)[slot_], *source_, f);
}

Value
curv::Nonlocal_Ref::eval(Frame& f) const
{
    return (*f.nonlocal)[slot_];
}

Value
curv::Let_Ref::eval(Frame& f) const
{
    // Let bindings are represented as slots in the frame, and are lazily
    // evaluated. The slots are initialized with thunks. On first reference,
    // the thunk is evaluated and the slot is updated with the resulting value.
    // Recursion is detected and causes an error.

    return force_ref(f[slot_], *source_, f);
}

Value
curv::Arg_Ref::eval(Frame& f) const
{
    return f[slot_];
}

Value
curv::Local_Function_Ref::eval(Frame& f) const
{
    return {aux::make_shared<Closure>(
        (Lambda&) f[lambda_slot_].get_ref_unsafe(),
        (List&) f[env_slot_].get_ref_unsafe())};
}

Value
curv::Nonlocal_Function_Ref::eval(Frame& f) const
{
    return {aux::make_shared<Closure>(
        (Lambda&) (*f.nonlocal)[lambda_slot_].get_ref_unsafe(),
        *f.nonlocal)};
}

Value
curv::Call_Expr::eval(Frame& f) const
{
    Value funv = curv::eval(*fun_, f);
    if (!funv.is_ref())
        throw Exception(At_Phrase(*fun_->source_, &f),
            stringify(funv,": not a function"));
    Ref_Value& funp( funv.get_ref_unsafe() );
    // TODO: these two cases are so similar, can we unify them?
    switch (funp.type_) {
    case Ref_Value::ty_function:
      {
        Function* fun = (Function*)&funp;
        if (args_.size() != fun->nargs_) {
            throw Exception(At_Phrase(*argsource_, &f),
                "wrong number of arguments");
        }
        std::unique_ptr<Frame> f2
            { Frame::make(fun->nargs_, &f, &*source_, nullptr) };
        for (size_t i = 0; i < args_.size(); ++i)
            (*f2)[i] = curv::eval(*args_[i], f);
        return fun->function_(*f2);
      }
    case Ref_Value::ty_closure:
      {
        Closure* fun = (Closure*)&funp;
        if (args_.size() != fun->nargs_) {
            throw Exception(At_Phrase(*argsource_, &f),
                "wrong number of arguments");
        }
        std::unique_ptr<Frame> f2
            { Frame::make(fun->nslots_, &f, &*source_, &*fun->nonlocals_) };
        for (size_t i = 0; i < args_.size(); ++i)
            (*f2)[i] = curv::eval(*args_[i], f);
        return fun->expr_->eval(*f2);
      }
    default:
        throw Exception(At_Phrase(*fun_->source_, &f),
            stringify(funv,": not a function"));
    }
}

Value
curv::Dot_Expr::eval(Frame& f) const
{
    Value basev = curv::eval(*base_, f);
    if (!basev.is_ref())
        throw Exception(At_Phrase(*base_->source_, &f),
            "not a record or module");
    Ref_Value& basep( basev.get_ref_unsafe() );
    switch (basep.type_) {
    case Ref_Value::ty_record:
      {
        Record* record = (Record*)&basep;
        auto fp = record->fields_.find(id_);
        if (fp != record->fields_.end())
            return fp->second;
        throw Exception(At_Phrase(*base_->source_, &f),
            stringify(".",id_,": not defined"));
      }
    case Ref_Value::ty_module:
      {
        Module* module = (Module*)&basep;
        auto b = module->dictionary_->find(id_);
        if (b != module->dictionary_->end())
            return module->get(b->second);
        throw Exception(At_Phrase(*base_->source_, &f),
            stringify(".",id_,": not defined"));
      }
    default:
        throw Exception(At_Phrase(*base_->source_, &f),
            "not a record or module");
    }
}

Value
curv::Prefix_Expr::eval(Frame& f) const
{
    switch (op_) {
    case Token::k_minus:
        {
            Value a = curv::eval(*arg_, f);
            Value r = Value(-a.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify("-",a,": domain error"));
            return r;
        }
    case Token::k_plus:
        {
            Value a = curv::eval(*arg_, f);
            Value r = Value(+a.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify("+",a,": domain error"));
            return r;
        }
    default:
        assert(0);
    }
}

Value
curv::Not_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg_, f);
    if (!a.is_bool())
        throw Exception(At_Phrase(*source_, &f),
            stringify("!",a,": domain error"));
    return {!a.get_bool_unsafe()};
}

Value
curv::Infix_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);

    switch (op_) {
    case Token::k_plus:
        {
            Value r = Value(a.get_num_or_nan() + b.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify(a,"+",b,": domain error"));
            return r;
        }
    case Token::k_minus:
        {
            Value r = Value(a.get_num_or_nan() - b.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify(a,"-",b,": domain error"));
            return r;
        }
    case Token::k_times:
        {
            Value r = Value(a.get_num_or_nan() * b.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify(a,"*",b,": domain error"));
            return r;
        }
    case Token::k_over:
        {
            Value r = Value(a.get_num_or_nan() / b.get_num_or_nan());
            if (!r.is_num())
                throw Exception(At_Phrase(*source_, &f),
                    stringify(a,"/",b,": domain error"));
            return r;
        }
    default:
        assert(0);
    }
}

Value
curv::Or_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{true})
        return a;
    if (a == Value{false}) {
        Value b = curv::eval(*arg2_, f);
        if (b.is_bool())
            return b;
        throw Exception(At_Phrase(*arg2_->source_, &f), "not a boolean value");
    }
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
Value
curv::And_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{false})
        return a;
    if (a == Value{true}) {
        Value b = curv::eval(*arg2_, f);
        if (b.is_bool())
            return b;
        throw Exception(At_Phrase(*arg2_->source_, &f), "not a boolean value");
    }
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
Value
curv::If_Expr::eval(Frame& f) const
{
    throw Exception{At_Phrase{*source_, &f},
        "if: not an expression (missing else clause)"};
}
void
curv::If_Expr::generate(Frame& f, List_Builder& lb) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{true})
        arg2_->generate(f, lb);
    else if (a == Value{false})
        return;
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
Value
curv::If_Else_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{true})
        return curv::eval(*arg2_, f);
    if (a == Value{false})
        return curv::eval(*arg3_, f);
    throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
void
curv::If_Else_Expr::generate(Frame& f, List_Builder& lb) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{true})
        arg2_->generate(f, lb);
    else if (a == Value{false})
        arg3_->generate(f, lb);
    else
        throw Exception(At_Phrase(*arg1_->source_, &f), "not a boolean value");
}
Value
curv::Equal_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    return {a == b};
}
Value
curv::Not_Equal_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    return {a != b};
}
Value
curv::Less_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,"<",b,": domain error"));
}
Value
curv::Greater_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,">",b,": domain error"));
}
Value
curv::Less_Or_Equal_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,"<=",b,": domain error"));
}
Value
curv::Greater_Or_Equal_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {false};
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,">=",b,": domain error"));
}
Value
curv::Power_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    Value r = Value(pow(a.get_num_or_nan(), b.get_num_or_nan()));
    if (r.is_num())
        return r;
    throw Exception(At_Phrase(*source_, &f),
        stringify(a,"^",b,": domain error"));
}
Value
curv::At_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    auto& list {arg_to_list(a, At_Phrase(*arg1_->source_, &f))};
    int i =
        arg_to_int(b, 0, (int)(list.size()-1), At_Phrase(*arg2_->source_, &f));
    return list[i];
}

Shared<List>
curv::List_Expr_Base::eval_list(Frame& f) const
{
    // TODO: This used to have a more efficient implementation, assuming that
    // all elements of the list constructor are pure expressions. An optimizing
    // compiler could bring that back as a special case.
    List_Builder lb;
    for (size_t i = 0; i < this->size(); ++i)
        (*this)[i]->generate(f, lb);
    return lb.get_list();
}

Value
curv::List_Expr_Base::eval(Frame& f) const
{
    return {eval_list(f)};
}

Value
curv::Record_Expr::eval(Frame& f) const
{
    auto record = aux::make_shared<Record>();
    for (auto i : fields_)
        record->fields_[i.first] = curv::eval(*i.second, f);
    return {record};
}

Value
curv::Module_Expr::eval(Frame& f) const
{
    auto module = eval_module(&f);
    return {module};
}

Shared<Module>
curv::Module_Expr::eval_module(Frame* f) const
{
    auto module = aux::make_shared<Module>();
    module->dictionary_ = dictionary_;
    module->slots_ = List::make_copy(slots_->begin(), slots_->size());
    std::unique_ptr<Frame> frame
        {Frame::make(frame_nslots_, f, nullptr, &*module->slots_)};
    for (Value& s : *module->slots_)
        force(s, *frame);
    module->elements_ = elements_->eval_list(*frame);
    return module;
}

Value
curv::Let_Expr::eval(Frame& f) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < values_.size(); ++i)
        slots[i] = values_[i];
    return curv::eval(*body_, f);
}
void
curv::Let_Expr::generate(Frame& f, List_Builder& lb) const
{
    Value* slots = &f[first_slot_];
    for (size_t i = 0; i < values_.size(); ++i)
        slots[i] = values_[i];
    body_->generate(f, lb);
}

Value
curv::For_Expr::eval(Frame& f) const
{
    throw Exception{At_Phrase{*source_, &f}, "not an expression"};
}

void
curv::For_Expr::generate(Frame& f, List_Builder& lb) const
{
    Value listval = curv::eval(*list_, f);
    List& list = arg_to_list(listval, At_Phrase{*list_->source_, &f});
    for (size_t i = 0; i < list.size(); ++i) {
        f[slot_] = list[i];
        body_->generate(f, lb);
    }
}

Value
curv::Lambda_Expr::eval(Frame& f) const
{
    return Value{aux::make_shared<Closure>(
        body_,
        nonlocals_->eval_list(f),
        nargs_,
        nslots_)};
}
