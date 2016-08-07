// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/meaning.h>
#include <curv/eval.h>
#include <curv/string.h>
#include <curv/exception.h>
#include <curv/function.h>
#include <curv/list.h>
#include <curv/record.h>
#include <curv/module.h>
#include <cmath>

using namespace curv;

Value
curv::Constant::eval() const
{
    return value_;
}

Value
curv::Call_Expr::eval() const
{
    Value funv = curv::eval(*fun_);
    if (!funv.is_ref())
        throw Phrase_Error(*fun_->source_,
            stringify(funv,": not a function"));
    Ref_Value& funp( funv.get_ref_unsafe() );
    if (funp.type_ != Ref_Value::ty_function)
        throw Phrase_Error(*fun_->source_,
            stringify(funv,": not a function"));
    Function* fun = (Function*)&funp;

    Value argv[1];
    switch (fun->nargs_) {
    case 1:
        if (args_.size() != 1) {
            throw Phrase_Error(*argsource_,
                "wrong number of arguments");
        }
        argv[0] = curv::eval(*args_[0]);
        return fun->function_(argv);
    default:
        throw Phrase_Error(*source_,
            stringify("unsupported argument list size ",args_.size()));
    }
}

Value
curv::Dot_Expr::eval() const
{
    Value basev = curv::eval(*base_);
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
curv::Prefix_Expr::eval() const
{
    switch (op_) {
    case Token::k_minus:
        {
            Value a = curv::eval(*arg_);
            Value r = Value(-a.get_num_or_nan());
            if (!r.is_num())
                throw Phrase_Error(*source_,
                    stringify("-",a,": domain error"));
            return r;
        }
    case Token::k_plus:
        {
            Value a = curv::eval(*arg_);
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
curv::Not_Expr::eval() const
{
    Value a = curv::eval(*arg_);
    if (!a.is_bool())
        throw Phrase_Error(*source_,
            stringify("!",a,": domain error"));
    return {!a.get_bool_unsafe()};
}

Value
curv::Infix_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);

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
curv::Or_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    if (a == Value{true})
        return a;
    if (a == Value{false}) {
        Value b = curv::eval(*arg2_);
        if (b.is_bool())
            return b;
        throw Phrase_Error(*arg2_->source_, "not a boolean value");
    }
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
}
Value
curv::And_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    if (a == Value{false})
        return a;
    if (a == Value{true}) {
        Value b = curv::eval(*arg2_);
        if (b.is_bool())
            return b;
        throw Phrase_Error(*arg2_->source_, "not a boolean value");
    }
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
}
Value
curv::If_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    if (a == Value{true})
        return curv::eval(*arg2_);
    if (a == Value{false})
        return curv::eval(*arg3_);
    throw Phrase_Error(*arg1_->source_, "not a boolean value");
}
Value
curv::Equal_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    return {a == b};
}
Value
curv::Not_Equal_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    return {a != b};
}
Value
curv::Less_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {false};
    throw Phrase_Error(*source_,
        stringify(a,"<",b,": domain error"));
}
Value
curv::Greater_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {false};
    throw Phrase_Error(*source_,
        stringify(a,">",b,": domain error"));
}
Value
curv::Less_Or_Equal_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() <= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() > b.get_num_or_nan())
        return {false};
    throw Phrase_Error(*source_,
        stringify(a,"<=",b,": domain error"));
}
Value
curv::Greater_Or_Equal_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    // only 2 comparisons required to unbox two numbers and compare them, not 3
    if (a.get_num_or_nan() >= b.get_num_or_nan())
        return {true};
    if (a.get_num_or_nan() < b.get_num_or_nan())
        return {false};
    throw Phrase_Error(*source_,
        stringify(a,">=",b,": domain error"));
}
Value
curv::Power_Expr::eval() const
{
    Value a = curv::eval(*arg1_);
    Value b = curv::eval(*arg2_);
    Value r = Value(pow(a.get_num_or_nan(), b.get_num_or_nan()));
    if (r.is_num())
        return r;
    throw Phrase_Error(*source_, stringify(a,"^",b,": domain error"));
}

Shared<List>
curv::List_Expr_Base::eval_list() const
{
    auto list = make_list(this->size());
    for (size_t i = 0; i < this->size(); ++i)
        (*list)[i] = curv::eval(*(*this)[i]);
    return list;
}

Value
curv::List_Expr_Base::eval() const
{
    return {eval_list()};
}

Value
curv::Record_Expr::eval() const
{
    auto record = aux::make_shared<Record>();
    for (auto i : fields_)
        record->fields_[i.first] = curv::eval(*i.second);
    return {record};
}

Value
curv::Module_Expr::eval() const
{
    auto module = aux::make_shared<Module>();
    for (auto i : fields_)
        module->fields_[i.first] = curv::eval(*i.second);
    module->elements_ = elements_->eval_list();
    return {module};
}
