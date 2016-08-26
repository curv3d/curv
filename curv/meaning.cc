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
#include <cmath>

using namespace curv;

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

    return force_ref(f.module_.fields_[atom_], *source_, f);
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
curv::Call_Expr::eval(Frame& f) const
{
    Value funv = curv::eval(*fun_, f);
    if (!funv.is_ref())
        throw Phrase_Error(*fun_->source_, stringify(funv,": not a function"));
    Ref_Value& funp( funv.get_ref_unsafe() );
    switch (funp.type_) {
    case Ref_Value::ty_function:
      {
        Function* fun = (Function*)&funp;
        Value argv[1];
        switch (fun->nargs_) {
        case 1:
            if (args_.size() != 1) {
                throw Phrase_Error(*argsource_,
                    "wrong number of arguments");
            }
            argv[0] = curv::eval(*args_[0], f);
            return fun->function_(argv);
        default:
            throw Phrase_Error(*source_,
                stringify("unsupported argument list size ",args_.size()));
        }
      }
    case Ref_Value::ty_lambda:
      {
        // currently, lambdas accept exactly 1 argument
        Lambda* fun = (Lambda*)&funp;
        if (args_.size() != 1) {
            throw Phrase_Error(*argsource_,
                "wrong number of arguments");
        }
        std::unique_ptr<Frame> f2 { Frame::make(fun->nslots_, f.module_) };
        (*f2)[0] = curv::eval(*args_[0], f);
        return fun->expr_->eval(*f2);
      }
    default:
        throw Phrase_Error(*fun_->source_, stringify(funv,": not a function"));
    }
}

Value
curv::Dot_Expr::eval(Frame& f) const
{
    Value basev = curv::eval(*base_, f);
    if (!basev.is_ref())
        throw Phrase_Error(*base_->source_, "not a record or module");
    Ref_Value& basep( basev.get_ref_unsafe() );
    switch (basep.type_) {
    case Ref_Value::ty_record:
      {
        Record* record = (Record*)&basep;
        auto f = record->fields_.find(id_);
        if (f != record->fields_.end())
            return f->second;
        throw Phrase_Error(*base_->source_,
            stringify(".",id_,": not defined"));
      }
    case Ref_Value::ty_module:
      {
        Module* module = (Module*)&basep;
        auto f = module->fields_.find(id_);
        if (f != module->fields_.end())
            return f->second;
        throw Phrase_Error(*base_->source_,
            stringify(".",id_,": not defined"));
      }
    default:
        throw Phrase_Error(*base_->source_, "not a record or module");
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
                throw Phrase_Error(*source_,
                    stringify("-",a,": domain error"));
            return r;
        }
    case Token::k_plus:
        {
            Value a = curv::eval(*arg_, f);
            Value r = Value(+a.get_num_or_nan());
            if (!r.is_num())
                throw Phrase_Error(*source_,
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
        throw Phrase_Error(*source_,
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
                throw Phrase_Error(*source_,
                    stringify(a,"+",b,": domain error"));
            return r;
        }
    case Token::k_minus:
        {
            Value r = Value(a.get_num_or_nan() - b.get_num_or_nan());
            if (!r.is_num())
                throw Phrase_Error(*source_,
                    stringify(a,"-",b,": domain error"));
            return r;
        }
    case Token::k_times:
        {
            Value r = Value(a.get_num_or_nan() * b.get_num_or_nan());
            if (!r.is_num())
                throw Phrase_Error(*source_,
                    stringify(a,"*",b,": domain error"));
            return r;
        }
    case Token::k_over:
        {
            Value r = Value(a.get_num_or_nan() / b.get_num_or_nan());
            if (!r.is_num())
                throw Phrase_Error(*source_,
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
        throw Phrase_Error(*arg2_->source_, "not a boolean value");
    }
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
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
        throw Phrase_Error(*arg2_->source_, "not a boolean value");
    }
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
}
Value
curv::If_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    if (a == Value{true})
        return curv::eval(*arg2_, f);
    if (a == Value{false})
        return curv::eval(*arg3_, f);
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
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
    throw Phrase_Error(*source_,
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
    throw Phrase_Error(*source_,
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
    throw Phrase_Error(*source_,
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
    throw Phrase_Error(*source_,
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
    throw Phrase_Error(*source_, stringify(a,"^",b,": domain error"));
}
Value
curv::At_Expr::eval(Frame& f) const
{
    Value a = curv::eval(*arg1_, f);
    Value b = curv::eval(*arg2_, f);
    auto& list {arg_to_list(a, *arg1_->source_)};
    int i = arg_to_int(b, 0, (int)(list.size()-1), *arg2_->source_);
    return list[i];
}

Shared<List>
curv::List_Expr_Base::eval_list(Frame& f) const
{
    auto list = make_list(this->size());
    for (size_t i = 0; i < this->size(); ++i)
        (*list)[i] = curv::eval(*(*this)[i], f);
    return list;
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
curv::Module_Expr::eval(Frame&) const
{
    auto module = eval_module();
    return {module};
}

Shared<Module>
curv::Module_Expr::eval_module() const
{
    auto module = aux::make_shared<Module>();
    std::unique_ptr<Frame> f { Frame::make(frame_nslots_, *module) };
    for (auto i : fields_)
        module->fields_[i.first] = {aux::make_shared<Thunk>(i.second)};
    for (auto i : fields_)
        force(module->fields_[i.first], *f);
    module->elements_ = elements_->eval_list(*f);
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
