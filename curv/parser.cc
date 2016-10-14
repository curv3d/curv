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

Shared<Phrase> parse_expr(Scanner& scanner);
Shared<Phrase> parse_disjunction(Scanner&);
Shared<Phrase> parse_conjunction(Scanner&);
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
// stmts : expr | stmts ; expr
Shared<Module_Phrase>
parse_script(Scanner& scanner)
{
    auto module = aux::make_shared<Module_Phrase>(scanner.script_);
    Token tok = scanner.get_token();
    for (;;) {
        if (tok.kind == Token::k_end)
            break;
        scanner.push_token(tok);
        Module_Phrase::Statement stmt(parse_expr(scanner));
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

// Low precedence right associative infix operators:
//
// expr : disjunction | definition | lambda
// definition : id = expr
// lambda : primary -> expr
Shared<Phrase>
parse_expr(Scanner& scanner)
{
    auto left = parse_disjunction(scanner);
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_equate:
        return aux::make_shared<Definition>(
            std::move(left), tok, parse_expr(scanner));
    case Token::k_right_arrow:
        return aux::make_shared<Lambda_Phrase>(
            std::move(left), tok, parse_expr(scanner));
    default:
        scanner.push_token(tok);
        return left;
    }
}

// disjunction : conjunction | disjunction || conjunction
Shared<Phrase>
parse_disjunction(Scanner& scanner)
{
    auto left = parse_conjunction(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_or:
            left = aux::make_shared<Binary_Phrase>(
                std::move(left), tok, parse_conjunction(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// conjunction : relation | conjunction && relation
Shared<Phrase>
parse_conjunction(Scanner& scanner)
{
    auto left = parse_relation(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_and:
            left = aux::make_shared<Binary_Phrase>(
                std::move(left), tok, parse_relation(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// relation : sum
//  | sum == sum | sum != sum
//  | sum < sum | sum > sum
//  | sum <= sum | sum >= sum
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
//  | postfix . list
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
            postfix = aux::make_shared<Binary_Phrase>(postfix, tok,
                parse_primary(scanner, true));
            continue;
        }
        scanner.push_token(tok);
        auto primary = parse_primary(scanner, false);
        if (primary == nullptr)
            return postfix;
        postfix = aux::make_shared<Call_Phrase>(postfix, primary);
    }
}

// primary : numeral | identifier | string | parens | list | record
//  | 'if' ( expr ) expr 'else' expr
//  | 'let' parens expr
// parens : ( args )
// list : [ args ]
// record : { args }
// args : ( ) | ( arglist ) | ( arglist , )
// arglist : expr | arglist , expr
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
    case Token::k_if:
      {
        auto condition = parse_primary(scanner, true);
        if (dynamic_cast<Paren_Phrase*>(condition.get()) == nullptr)
            throw Phrase_Error(*condition, "not a parenthesized expression");
        auto then_expr = parse_expr(scanner);
        Token tok2 = scanner.get_token();
        if (tok2.kind != Token::k_else)
            throw Token_Error(scanner.script_, tok2, "not the keyword 'else'");
        auto else_expr = parse_expr(scanner);
        return aux::make_shared<If_Phrase>(
            tok, condition, then_expr, tok2, else_expr);
      }
    case Token::k_let:
      {
        auto p = parse_primary(scanner, true);
        auto args = dynamic_shared_cast<Paren_Phrase>(p);
        if (args == nullptr)
            throw Phrase_Error(*p, "not a parenthesized expression");
        auto expr = parse_expr(scanner);
        return aux::make_shared<Let_Phrase>(tok, args, expr);
      }
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
            auto expr = parse_expr(scanner);
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
