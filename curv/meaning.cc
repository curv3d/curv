// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/meaning.h>
#include <curv/eval.h>
#include <curv/string.h>
#include <curv/exception.h>
#include <curv/function.h>

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
