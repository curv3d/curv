// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <curv/eval.h>
#include <curv/exception.h>
#include <boost/math/constants/constants.hpp>

using namespace curv;
using namespace std;
using namespace boost::math::double_constants;

const Namespace
curv::builtin_namespace = {
    {"pi", pi},
    {"tau", two_pi},
    {"inf", INFINITY},
};

double
curv::eval(Syntax& expr, const Namespace& names)
{
    // Hmm. Obviously, this could be done using a virtual eval function.
    // But, this code is temporary scaffolding, so I'll wait.

    auto num = dynamic_cast<NumExpr*>(&expr);
    if (num != nullptr) {
        string str(num->numeral.begin(), num->numeral.size());
        char* endptr;
        double n = strtod(str.c_str(), &endptr);
        assert(endptr == str.c_str() + str.size());
        return n;
    }

    auto ident = dynamic_cast<IdentExpr*>(&expr);
    if (ident != nullptr) {
        std::string id(ident->identifier.begin(), ident->identifier.size());
        auto p = names.find(id);
        if (p != names.end())
            return p->second;
        else
            throw SyntaxError(ident->identifier, "not defined");
    }

    auto unary = dynamic_cast<UnaryExpr*>(&expr);
    if (unary != nullptr) {
        switch (unary->optor.kind) {
        case Token::k_minus:
            return -eval(*unary->argument, names);
        case Token::k_plus:
            return +eval(*unary->argument, names);
        default:
            assert(0);
        }
    }

    auto binary = dynamic_cast<BinaryExpr*>(&expr);
    if (binary != nullptr) {
        switch (binary->optor.kind) {
        case Token::k_plus:
            return eval(*binary->left, names) + eval(*binary->right, names);
        case Token::k_minus:
            return eval(*binary->left, names) - eval(*binary->right, names);
        case Token::k_times:
            return eval(*binary->left, names) * eval(*binary->right, names);
        case Token::k_over:
            return eval(*binary->left, names) / eval(*binary->right, names);
        default:
            assert(0);
        }
    }

    auto paren = dynamic_cast<ParenExpr*>(&expr);
    if (paren != nullptr) {
        return eval(*paren->argument, names);
    }

    assert(0);
}
