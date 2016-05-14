/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#include <cassert>
#include <cstdlib>
#include <string>
#include <curv/eval.h>

using namespace curv;
using namespace std;

double
curv::eval(Expr& expr)
{
    // Hmm. Obviously, this could be done using a virtual eval function.
    // But, this code is temporary scaffolding, so I'll wait.

    auto num = dynamic_cast<NumExpr*>(&expr);
    if (num != nullptr) {
        string str(num->numeral.begin(), num->numeral.size());
        return strtod(str.c_str(), nullptr);
    }

    auto unary = dynamic_cast<UnaryExpr*>(&expr);
    if (unary != nullptr) {
        switch (unary->optor.kind) {
        case Token::k_minus:
            return -eval(*unary->argument);
        case Token::k_plus:
            return +eval(*unary->argument);
        default:
            assert(0);
        }
    }

    auto binary = dynamic_cast<BinaryExpr*>(&expr);
    if (binary != nullptr) {
        switch (binary->optor.kind) {
        case Token::k_plus:
            return eval(*binary->left) + eval(*binary->right);
        case Token::k_minus:
            return eval(*binary->left) - eval(*binary->right);
        case Token::k_times:
            return eval(*binary->left) * eval(*binary->right);
        case Token::k_over:
            return eval(*binary->left) / eval(*binary->right);
        default:
            assert(0);
        }
    }

    auto paren = dynamic_cast<ParenExpr*>(&expr);
    if (paren != nullptr) {
        return eval(*paren->argument);
    }

    assert(0);
}
