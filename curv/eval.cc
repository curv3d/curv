// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/function.h>
#include <boost/math/constants/constants.hpp>

using namespace curv;
using namespace std;
using namespace boost::math::double_constants;

Value
builtin_sqrt(Value* args)
{
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw aux::Exception(stringify("sqrt(",args[0],"): domain error"));
}

const Namespace
curv::builtin_namespace = {
    {"pi", pi},
    {"tau", two_pi},
    {"inf", INFINITY},
    {"null", curv::Value()},
    {"false", curv::Value(false)},
    {"true", curv::Value(true)},
    {"sqrt", curv::make_ref_value<curv::Function>(builtin_sqrt, 1)},
};

Value
curv::eval(Phrase& expr, const Namespace& names)
{
    // Hmm. Obviously, this could be done using a virtual eval function.
    // But, this code is temporary scaffolding, so I'll wait.

    auto num = dynamic_cast<Numeral*>(&expr);
    if (num != nullptr) {
        string str(num->location().range());
        char* endptr;
        double n = strtod(str.c_str(), &endptr);
        assert(endptr == str.c_str() + str.size());
        return n;
    }

    auto ident = dynamic_cast<Identifier*>(&expr);
    if (ident != nullptr) {
        std::string id(ident->location().range());
        auto p = names.find(id);
        if (p != names.end())
            return p->second;
        else
            throw Phrase_Error(*ident, "not defined");
    }

    auto unary = dynamic_cast<Unary_Phrase*>(&expr);
    if (unary != nullptr) {
        switch (unary->op_.kind) {
        case Token::k_minus:
            {
                Value a = eval(*unary->arg_, names);
                Value r = Value(-a.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*unary,
                        stringify("-",a,": domain error"));
                return r;
            }
        case Token::k_plus:
            {
                Value a = eval(*unary->arg_, names);
                Value r = Value(+a.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*unary,
                        stringify("+",a,": domain error"));
                return r;
            }
        default:
            assert(0);
        }
    }

    auto binary = dynamic_cast<Binary_Phrase*>(&expr);
    if (binary != nullptr) {
        switch (binary->op_.kind) {
        case Token::k_plus:
            {
                Value a = eval(*binary->left_, names);
                Value b = eval(*binary->right_, names);
                Value r = Value(a.get_num_or_nan() + b.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*binary,
                        stringify(a,"+",b,": domain error"));
                return r;
            }
        case Token::k_minus:
            {
                Value a = eval(*binary->left_, names);
                Value b = eval(*binary->right_, names);
                Value r = Value(a.get_num_or_nan() - b.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*binary,
                        stringify(a,"-",b,": domain error"));
                return r;
            }
        case Token::k_times:
            {
                Value a = eval(*binary->left_, names);
                Value b = eval(*binary->right_, names);
                Value r = Value(a.get_num_or_nan() * b.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*binary,
                        stringify(a,"*",b,": domain error"));
                return r;
            }
        case Token::k_over:
            {
                Value a = eval(*binary->left_, names);
                Value b = eval(*binary->right_, names);
                Value r = Value(a.get_num_or_nan() / b.get_num_or_nan());
                if (!r.is_num())
                    throw Phrase_Error(*binary,
                        stringify(a,"/",b,": domain error"));
                return r;
            }
        default:
            assert(0);
        }
    }

    auto call = dynamic_cast<Call_Phrase*>(&expr);
    if (call != nullptr) {
        Value funv = eval(*call->function_, names);
        if (!funv.is_ref())
            throw Phrase_Error(*call->function_,
                stringify(funv,": not a function"));
        Ref_Value& funp( funv.get_ref_unsafe() );
        if (funp.type_ != Ref_Value::ty_function)
            throw Phrase_Error(*call->function_,
                stringify(funv,": not a function"));
        Function* fun = (Function*)&funp;

        const auto& args(call->arglist_->args_);
        Value argv[1];
        switch (fun->nargs_) {
        case 1:
            if (args.size() != 1) {
                throw Phrase_Error(*call->arglist_,
                    "wrong number of arguments");
            }
            argv[0] = eval(*args[0].expr_, names);
            return fun->function_(argv);
        default:
            throw Phrase_Error(*call, "unsupported argument list size");
        }
    }

    auto paren = dynamic_cast<Paren_Phrase*>(&expr);
    if (paren != nullptr) {
        return eval(*paren->arg_, names);
    }

    assert(0);
}
