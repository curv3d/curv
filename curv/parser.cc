// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

// IMPLEMENTATION NOTE:
// The current plan is to have 2 entry points to the parser, one for parsing
// interactive command lines, and one for parsing script files.
// I don't know how to implement 2 entry points using a parser generator
// (Bison or Lemon). For now, I'm using a hand coded recursive descent parser.
// I may switch to Boost.Spirit.

#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/exception.h>

using namespace std;
using namespace curv;
using namespace aux;

namespace curv {

Shared_Ptr<Phrase> parse_stmt(Scanner& scanner);
Shared_Ptr<Phrase> parse_sum(Scanner&);
Shared_Ptr<Phrase> parse_product(Scanner&);
Shared_Ptr<Phrase> parse_unary(Scanner&);
Shared_Ptr<Phrase> parse_postfix(Scanner&);
Shared_Ptr<Phrase> parse_atom(Scanner&);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
/*
stmt : definition | sum
definition : id = sum
sum : product | sum + product | sum - product
product : unary | product * unary | product / unary
unary : postfix | - unary | + unary
postfix : atom | postfix ( args )
atom : numeral | ( sum )
*/

/// Parse a tcad command line.
///
/// Returns `nullptr` for an empty line.
/// Returns `Shared_Ptr<Definition>` for `id = expr`.
/// Otherwise, returns an expression.
Shared_Ptr<Phrase>
parse(const Script& script)
{
    Scanner scanner(script);

    auto tok = scanner.get_token();
    if (tok.kind == Token::k_end)
        return nullptr;
    scanner.push_token(tok);

    auto stmt = parse_stmt(scanner);
    tok = scanner.get_token();
    if (tok.kind != Token::k_end)
        throw Token_Error(script, tok, "unexpected token at end of script");
    return stmt;
}

// stmt : definition | sum
// definition : id = sum
Shared_Ptr<Phrase>
parse_stmt(Scanner& scanner)
{
    auto left = parse_sum(scanner);
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_equate) {
        auto right = parse_sum(scanner);
        return aux::make_shared<Definition>(left, tok, right);
    } else {
        scanner.push_token(tok);
        return left;
    }
}

// sum : product | sum + product | sum - product
Shared_Ptr<Phrase>
parse_sum(Scanner& scanner)
{
    auto left = parse_product(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_plus:
        case Token::k_minus:
            left = aux::make_shared<Binary_Phrase>(
                std::move(left), tok, parse_product(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// product : unary | product * unary | product / unary
Shared_Ptr<Phrase>
parse_product(Scanner& scanner)
{
    auto left = parse_unary(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_times:
        case Token::k_over:
            left = aux::make_shared<Binary_Phrase>(
                std::move(left), tok, parse_unary(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// unary : postfix | - unary | + unary
Shared_Ptr<Phrase>
parse_unary(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_plus:
    case Token::k_minus:
        return aux::make_shared<Unary_Phrase>(tok, parse_unary(scanner));
    default:
        scanner.push_token(tok);
        return parse_postfix(scanner);
    }
}

// postfix : atom | postfix arglist
// arglist : ( ) | ( args ) | ( args , )
// args : sum | args , sum
Shared_Ptr<Phrase>
parse_postfix(Scanner& scanner)
{
    auto postfix = parse_atom(scanner);
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        if (tok.kind == Token::k_lparen) {
            auto arglist =
                aux::make_shared<Arglist_Phrase>(scanner.script_, tok);
            for (;;) {
                tok = scanner.get_token();
                if (tok.kind == Token::k_rparen) {
            rparen:
                    arglist->rparen_ = tok;
                    postfix = aux::make_shared<Apply_Phrase>(postfix, arglist);
                    break;
                }
                scanner.push_token(tok);
                auto expr = parse_sum(scanner);
                Arglist_Phrase::Arg arg(expr);
                tok = scanner.get_token();
                if (tok.kind == Token::k_comma)
                    arg.comma_ = tok;
                arglist->args_.push_back(std::move(arg));

                if (tok.kind == Token::k_comma) {
                    continue;
                } else if (tok.kind == Token::k_rparen) {
                    goto rparen;
                } else {
                    throw Token_Error(scanner.script_, tok,
                        "bad token in function call argument list");
                }
            }
        } else {
            scanner.push_token(tok);
            return postfix;
        }
    }
}

// atom : numeral | identifier | ( sum )
Shared_Ptr<Phrase>
parse_atom(Scanner& scanner)
{
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_num) {
        return aux::make_shared<Numeral>(scanner.script_, tok);
    }
    if (tok.kind == Token::k_ident) {
        return aux::make_shared<Identifier>(scanner.script_, tok);
    }
    if (tok.kind == Token::k_lparen) {
        auto expr = parse_sum(scanner);
        auto tok2 = scanner.get_token();
        if (tok2.kind == Token::k_rparen)
            return aux::make_shared<Paren_Phrase>(tok, std::move(expr), tok2);
        throw Token_Error(scanner.script_, tok2,
            "unexpected token when expecting ')'");
    }
    throw Token_Error(scanner.script_, tok,
        "unexpected token when expecting atom");
}

}
