// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

// I'm currently using a hand coded recursive descent parser.
// Alternatives are: Bison, Lemon, Boost.Spirit (maybe X3).
//
// Future extensions:
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

Shared<Phrase> parse_stmt(Scanner& scanner);
Shared<Phrase> parse_relation(Scanner&);
Shared<Phrase> parse_sum(Scanner&);
Shared<Phrase> parse_product(Scanner&);
Shared<Phrase> parse_unary(Scanner&);
Shared<Phrase> parse_chain(Scanner&);
Shared<Phrase> parse_primary(Scanner&,bool);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
//
// script : | stmts | stmts ;
// stmts : stmt | stmts ; stmt
Shared<Phrase>
parse_script(Scanner& scanner)
{
    auto module = aux::make_shared<Module_Phrase>(scanner.script_);
    Token tok = scanner.get_token();
    for (;;) {
        if (tok.kind == Token::k_end)
            break;
        scanner.push_token(tok);
        Module_Phrase::Statement stmt(parse_stmt(scanner));
        tok = scanner.get_token();
        if (tok.kind == Token::k_semicolon) {
            stmt.semicolon_ = tok;
            tok = scanner.get_token();
        } else if (tok.kind != Token::k_end) {
            throw Token_Error(scanner.script_, tok,
                "bad token in module: expecting ';' or <end>");
        }
        module->stmts_.push_back(std::move(stmt));
    }
    module->end_ = tok;
    return module;
}

// stmt : definition | relation
// definition : id = relation
Shared<Phrase>
parse_stmt(Scanner& scanner)
{
    auto left = parse_relation(scanner);
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_equate) {
        auto right = parse_relation(scanner);
        return aux::make_shared<Definition>(left, tok, right);
    } else {
        scanner.push_token(tok);
        return left;
    }
}

// relation : sum | sum == sum
Shared<Phrase>
parse_relation(Scanner& scanner)
{
    auto left = parse_sum(scanner);
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_equal:
    case Token::k_not_equal:
    case Token::k_less:
    case Token::k_less_or_equal:
    case Token::k_greater:
    case Token::k_greater_or_equal:
        return aux::make_shared<Binary_Phrase>(
            std::move(left), tok, parse_sum(scanner));
    default:
        scanner.push_token(tok);
        return left;
    }
}

// sum : product | sum + product | sum - product
Shared<Phrase>
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
Shared<Phrase>
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

// unary : chain | - unary | + unary
Shared<Phrase>
parse_unary(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_plus:
    case Token::k_minus:
    case Token::k_not:
        return aux::make_shared<Unary_Phrase>(tok, parse_unary(scanner));
    default:
        scanner.push_token(tok);
        return parse_chain(scanner);
    }
}

// chain
//  : postfix
//  | postfix chain{begins-with-identifier}
//  | postfix ^ unary
// postfix
//  : primary
//  | postfix primary{not-identifier}
//  | postfix . identifier
Shared<Phrase>
parse_chain(Scanner& scanner)
{
    auto postfix = parse_primary(scanner, true);
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        if (tok.kind == Token::k_ident) {
            scanner.push_token(tok);
            return aux::make_shared<Call_Phrase>(postfix,
                parse_chain(scanner));
        }
        if (tok.kind == Token::k_power) {
            return aux::make_shared<Binary_Phrase>(
                postfix, tok, parse_unary(scanner));
        }
        if (tok.kind == Token::k_dot) {
            auto tok2 = scanner.get_token();
            if (tok2.kind != Token::k_ident) {
                throw Token_Error(scanner.script_, tok2,
                    ". operator not followed by identifier");
            }
            postfix = aux::make_shared<Dot_Phrase>(postfix, tok,
                aux::make_shared<Identifier>(scanner.script_, tok2));
            continue;
        }
        scanner.push_token(tok);
        auto primary = parse_primary(scanner, false);
        if (primary == nullptr)
            return postfix;
        postfix = aux::make_shared<Call_Phrase>(postfix, primary);
    }
}

// primary : numeral | identifier | parens
// parens : ( ) | ( args ) | ( args , )
// args : sum | args , sum
//
// If `force` is false, then we are parsing an optional primary,
// and we return nullptr if no primary is found.
Shared<Phrase>
parse_primary(Scanner& scanner, bool force)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_num:
        return aux::make_shared<Numeral>(scanner.script_, tok);
    case Token::k_ident:
        return aux::make_shared<Identifier>(scanner.script_, tok);
    case Token::k_string:
        return aux::make_shared<String_Phrase>(scanner.script_, tok);
    case Token::k_lparen:
    case Token::k_lbracket:
    case Token::k_lbrace:
        {
            Shared<Delimited_Phrase> parens;
            Token::Kind close;
            const char* error;
            if (tok.kind == Token::k_lparen) {
                parens = aux::make_shared<Paren_Phrase>(scanner.script_, tok);
                close = Token::k_rparen;
                error = "bad token in parenthesized phrase";
            } else if (tok.kind == Token::k_lbracket) {
                parens = aux::make_shared<List_Phrase>(scanner.script_, tok);
                close = Token::k_rbracket;
                error = "bad token in list constructor";
            } else if (tok.kind == Token::k_lbrace) {
                parens = aux::make_shared<Record_Phrase>(scanner.script_, tok);
                close = Token::k_rbrace;
                error = "bad token in record constructor";
            } else assert(0);
            for (;;) {
                tok = scanner.get_token();
                if (tok.kind == close) {
            rparen:
                    parens->rparen_ = tok;
                    return parens;
                }
                scanner.push_token(tok);
                auto expr = parse_stmt(scanner);
                Delimited_Phrase::Arg arg(expr);
                tok = scanner.get_token();
                if (tok.kind == Token::k_comma)
                    arg.comma_ = tok;
                parens->args_.push_back(std::move(arg));

                if (tok.kind == Token::k_comma) {
                    continue;
                } else if (tok.kind == close) {
                    goto rparen;
                } else {
                    throw Token_Error(scanner.script_, tok, error);
                }
            }
        }
        default:
            if (force) {
                throw Token_Error(scanner.script_, tok,
                    "unexpected token when expecting primary");
            } else {
                scanner.push_token(tok);
                return nullptr;
            }
    }
}

}
