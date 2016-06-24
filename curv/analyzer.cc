// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/shared.h>
#include <curv/exception.h>
using namespace curv;

Shared<Expression>
curv::analyze_expr(Phrase& ph, AContext ctx)
{
    Shared<Meaning> m = analyze(ph, ctx);
    Expression* e = dynamic_cast<Expression*>(&*m);
    if (e == nullptr)
        throw Phrase_Error(ph, "not an expression");
    return Shared<Expression>(e);
}

Shared<Meaning>
curv::Identifier::analyze(AContext ctx) const
{
    std::string id(location().range());
    auto p = ctx.names.find(id);
    if (p != ctx.names.end())
        return aux::make_shared<Constant>(
            Shared<const Phrase>(this), p->second);
    else
        throw Phrase_Error(*this, "not defined");
}

Shared<Meaning>
curv::Numeral::analyze(AContext ctx) const
{
    std::string str(location().range());
    char* endptr;
    double n = strtod(str.c_str(), &endptr);
    assert(endptr == str.c_str() + str.size());
    return aux::make_shared<Constant>(Shared<const Phrase>(this), n);
}

Shared<Meaning>
curv::Unary_Phrase::analyze(AContext ctx) const
{
    return aux::make_shared<Prefix_Expr>(
        Shared<const Phrase>(this),
        op_.kind,
        curv::analyze_expr(*arg_, ctx));
}

Shared<Meaning>
curv::Binary_Phrase::analyze(AContext ctx) const
{
    return aux::make_shared<Infix_Expr>(
        Shared<const Phrase>(this),
        op_.kind,
        curv::analyze_expr(*left_, ctx),
        curv::analyze_expr(*right_, ctx));
}

Shared<Meaning>
curv::Definition::analyze(AContext ctx) const
{
    throw Phrase_Error(*this, "not an expression");
}

Shared<Meaning>
curv::Paren_Phrase::analyze(AContext ctx) const
{
    if (args_.size() == 1
        && args_[0].comma_.kind == Token::k_missing)
    {
        return curv::analyze_expr(*args_[0].expr_, ctx);
    } else {
        throw Phrase_Error(*this, "not an expression");
    }
}

Shared<Meaning>
curv::Call_Phrase::analyze(AContext ctx) const
{
    auto fun = curv::analyze_expr(*function_, ctx);
    std::vector<Shared<Expression>> args;

    auto patom = dynamic_cast<Paren_Phrase*>(&*args_);
    if (patom != nullptr) {
        // argument phrase is a variable-length parenthesized argument list
        args.reserve(patom->args_.size());
        for (auto a : patom->args_)
            args.push_back(curv::analyze_expr(*a.expr_, ctx));
    } else {
        // argument phrase is a unitary expression
        args.reserve(1);
        args.push_back(curv::analyze_expr(*args_, ctx));
    }

    return aux::make_shared<Call_Expr>(
        Shared<const Phrase>(this),
        std::move(fun),
        args_,
        std::move(args));
}
