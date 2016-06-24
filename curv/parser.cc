// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

// I'm currently using a hand coded recursive descent parser.
// Alternatives are: Bison, Lemon, Boost.Spirit (maybe X3).
//
// Future extensions:
// * 2 entry points to the parser, one for parsing interactive command lines,
//   and one for parsing script files. This could be done in Bison using
//   2 magic zero-length start tokens.
// * Greedy implementation of 'if' and 'let' whose right expression argument
//   consumes the longest possible match. With Bison, the obvious implementation
//   is as a low precedence right associative operator. But, for more
//   flexibility, I want to implement these operators as part of 'chain'.
//   That's easy with recursive descent, but I don't know how to code that
//   in Bison.

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
Shared_Ptr<Phrase> parse_atom(Scanner&,bool);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.

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

// chain : postfix | postfix chain{begins-with-identifier}
// postfix : atom | postfix atom{not-identifier}
Shared_Ptr<Phrase>
parse_postfix(Scanner& scanner)
{
    auto postfix = parse_atom(scanner, true);
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        if (tok.kind == Token::k_ident) {
            scanner.push_token(tok);
            return aux::make_shared<Call_Phrase>(postfix,
                parse_postfix(scanner));
        }
        scanner.push_token(tok);
        auto atom = parse_atom(scanner, false);
        if (atom == nullptr)
            return postfix;
        postfix = aux::make_shared<Call_Phrase>(postfix, atom);
    }
}

// atom : numeral | identifier | patom
// patom : ( ) | ( args ) | ( args , )
// args : sum | args , sum
Shared_Ptr<Phrase>
parse_atom(Scanner& scanner, bool force)
{
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_num) {
        return aux::make_shared<Numeral>(scanner.script_, tok);
    }
    if (tok.kind == Token::k_ident) {
        return aux::make_shared<Identifier>(scanner.script_, tok);
    }
    if (tok.kind == Token::k_lparen) {
        auto pphrase = aux::make_shared<Paren_Phrase>(scanner.script_, tok);
        for (;;) {
            tok = scanner.get_token();
            if (tok.kind == Token::k_rparen) {
        rparen:
                pphrase->rparen_ = tok;
                return pphrase;
            }
            scanner.push_token(tok);
            auto expr = parse_sum(scanner);
            Paren_Phrase::Arg arg(expr);
            tok = scanner.get_token();
            if (tok.kind == Token::k_comma)
                arg.comma_ = tok;
            pphrase->args_.push_back(std::move(arg));

            if (tok.kind == Token::k_comma) {
                continue;
            } else if (tok.kind == Token::k_rparen) {
                goto rparen;
            } else {
                throw Token_Error(scanner.script_, tok,
                    "bad token in parenthesized phrase");
            }
        }
    }
    if (force) {
        throw Token_Error(scanner.script_, tok,
            "unexpected token when expecting atom");
    } else {
        scanner.push_token(tok);
        return nullptr;
    }
}

}
