// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

// I'm currently using a hand coded recursive descent parser.
// Alternatives are: Bison, Lemon, Boost.Spirit (maybe X3).
//
// I use a greedy implementation of 'if' and 'let': the right argument consumes
// the longest possible match. With Bison, the obvious implementation is as a
// low precedence right associative operator. But, for more flexibility,
// I implement these operators as part of 'chain'. That's easy with recursive
// descent, but I don't know how to code that in Bison.
//
// TODO: simple yaccable grammar, no conflicts or precedence declarations.
// TODO: $(expression) substitutions in string literals.
// TODO: `let` and `for` are metafunctions, not part of the grammar.
// TODO: infix `where` operator. Binds tighter than `->`.

#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/exception.h>
#include <curv/context.h>

using namespace std;
using namespace curv;
using namespace aux;

namespace curv {

Shared<Phrase> parse_semicolons(Scanner& scanner);
Shared<Phrase> parse_commas(Scanner& scanner);
Shared<Phrase> parse_expr(Scanner& scanner);
Shared<Phrase> parse_disjunction(Scanner&);
Shared<Phrase> parse_conjunction(Scanner&);
Shared<Phrase> parse_relation(Scanner&);
Shared<Phrase> parse_sum(Scanner&);
Shared<Phrase> parse_product(Scanner&);
Shared<Phrase> parse_unary(Scanner&);
Shared<Phrase> parse_chain(Scanner&);
Shared<Phrase> parse_primary(Scanner&,const char* what);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
//
// script : | stmts | stmts ;
// stmts : expr | stmts ; expr
Shared<Module_Phrase>
parse_script(Scanner& scanner)
{
    auto semis = parse_semicolons(scanner);
    Token tok = scanner.get_token();
    if (tok.kind != Token::k_end)
        throw Exception(At_Token(tok, scanner), "illegal token");
    return make<Module_Phrase>(semis, tok);
}

// semicolons : commas | semicolons `;` commas
//
// `;` is a left-associative infix operator,
// with the lowest operator precedence.
Shared<Phrase>
parse_semicolons(Scanner& scanner)
{
    auto left = parse_commas(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_semicolon:
            left = make<Semicolon_Phrase>(
                std::move(left), tok, parse_commas(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

bool
is_list_end_token(Token::Kind k)
{
    switch (k) {
    case Token::k_end:
    case Token::k_semicolon:
    case Token::k_rparen:
    case Token::k_rbracket:
    case Token::k_rbrace:
        return true;
    default:
        return false;
    }
}

// commas : empty | list | list `,`
// list : expr | list `,` expr
//
// How do I detect an empty commas phrase?
// * Peek at the first token, and if it is an end token from one of the
//   contexts where parse_commas is called, then report empty.
Shared<Phrase>
parse_commas(Scanner& scanner)
{
    auto tok = scanner.get_token();
    scanner.push_token(tok);
    Token begin = tok;
    begin.last = begin.first;
    auto commas = make<Comma_Phrase>(Location{scanner.script_, begin});
    for (;;) {
        // on entry, tok is lookahead of the next token
        if (is_list_end_token(tok.kind))
            return commas;
        auto expr = parse_expr(scanner);
        tok = scanner.get_token();
        if (tok.kind == Token::k_comma) {
            commas->args_.push_back({expr, tok});
            tok = scanner.get_token();
            scanner.push_token(tok);
        } else if (is_list_end_token(tok.kind)) {
            scanner.push_token(tok);
            if (commas->args_.empty())
                return expr;
            else {
                commas->args_.push_back({expr, {}});
                return commas;
            }
        } else
            throw Exception(At_Token(tok, scanner), "illegal token");
    }
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
        return make<Definition_Phrase>(
            std::move(left), tok, parse_expr(scanner));
    case Token::k_right_arrow:
        return make<Lambda_Phrase>(
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
            left = make<Binary_Phrase>(
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
            left = make<Binary_Phrase>(
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
//  | sum .. sum
//  | sum .. sum `step` sum
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
        return make<Binary_Phrase>(
            std::move(left), tok, parse_sum(scanner));
    case Token::k_range:
      {
        auto right = parse_sum(scanner);
        auto tok2 = scanner.get_token();
        Shared<Phrase> step;
        if (tok2.kind == Token::k_step) {
            step = parse_sum(scanner);
        } else {
            scanner.push_token(tok2);
            tok2 = {};
            step = nullptr;
        }
        return make<Range_Phrase>(
            std::move(left), tok, std::move(right), tok2, std::move(step));
      }
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
            left = make<Binary_Phrase>(
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
            left = make<Binary_Phrase>(
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
        return make<Unary_Phrase>(tok, parse_unary(scanner));
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
    auto postfix = parse_primary(scanner, "expression");
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        if (tok.kind == Token::k_ident) {
            scanner.push_token(tok);
            return make<Call_Phrase>(postfix,
                parse_chain(scanner));
        }
        if (tok.kind == Token::k_power) {
            return make<Binary_Phrase>(
                postfix, tok, parse_unary(scanner));
        }
        if (tok.kind == Token::k_dot) {
            postfix = make<Binary_Phrase>(postfix, tok,
                parse_primary(scanner, "expression following '.'"));
            continue;
        }
        scanner.push_token(tok);
        auto primary = parse_primary(scanner, nullptr);
        if (primary == nullptr)
            return postfix;
        postfix = make<Call_Phrase>(postfix, primary);
    }
}

template<class Ph>
Shared<Phrase>
parse_delimited(Token& tok, Token::Kind close, Scanner& scanner)
{
    auto body = parse_semicolons(scanner);
    auto tok2 = scanner.get_token();
    if (tok2.kind == Token::k_end)
        throw Exception(At_Token(tok, scanner), "unmatched delimiter");
    if (tok2.kind != close)
        throw Exception(At_Token(tok2, scanner), "illegal token");
    return make<Ph>(tok, body, tok2);
}

// primary : numeral | identifier | string | parens | list | braces
//  | 'if' ( expr ) expr
//  | 'if' ( expr ) expr 'else' expr
//  | 'let' parens expr
//  | 'for' parens expr
// parens : ( semicolons )
// list : [ semicolons ]
// braces : { semicolons }
//
// If `what` is nullptr, then we are parsing an optional primary,
// and we return nullptr if no primary is found.
// Otherwise, `what` is used to construct an error message if a primary
// expression isn't found.
Shared<Phrase>
parse_primary(Scanner& scanner, const char* what)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_num:
        return make<Numeral>(scanner.script_, tok);
    case Token::k_ident:
        return make<Identifier>(scanner.script_, tok);
    case Token::k_string:
        return make<String_Phrase>(scanner.script_, tok);
    case Token::k_if:
      {
        auto condition = parse_primary(scanner, "condition following 'if'");
        if (dynamic_cast<Paren_Phrase*>(condition.get()) == nullptr)
            throw Exception(At_Phrase(*condition, scanner.eval_frame_),
                "not a parenthesized expression");
        auto then_expr = parse_expr(scanner);
        Token tok2 = scanner.get_token();
        if (tok2.kind != Token::k_else) {
            scanner.push_token(tok2);
            return make<If_Phrase>(
                tok, condition, then_expr, Token{}, nullptr);
        }
        auto else_expr = parse_expr(scanner);
        return make<If_Phrase>(
            tok, condition, then_expr, tok2, else_expr);
      }
    case Token::k_let:
      {
        auto p = parse_primary(scanner, "argument following 'let'");
        auto args = dynamic_shared_cast<Paren_Phrase>(p);
        if (args == nullptr)
            throw Exception(At_Phrase(*p, scanner.eval_frame_),
                "let: malformed argument");
        auto body = parse_expr(scanner);
        return make<Let_Phrase>(tok, args, body);
      }
    case Token::k_for:
      {
        auto p = parse_primary(scanner, "argument following 'for'");
        auto args = dynamic_shared_cast<Paren_Phrase>(p);
        if (args == nullptr)
            throw Exception(At_Phrase(*p, scanner.eval_frame_),
                "for: malformed argument");
        auto body = parse_expr(scanner);
        return make<For_Phrase>(tok, args, body);
      }
    case Token::k_lparen:
        return parse_delimited<Paren_Phrase>(tok, Token::k_rparen, scanner);
    case Token::k_lbracket:
        return parse_delimited<List_Phrase>(tok, Token::k_rbracket, scanner);
    case Token::k_lbrace:
        return parse_delimited<Brace_Phrase>(tok, Token::k_rbrace, scanner);
    case Token::k_end:
        if (what != nullptr) {
            throw Exception(At_Token(tok, scanner),
                stringify("missing ", what));
        } else {
            scanner.push_token(tok);
            return nullptr;
        }
    default:
        if (what != nullptr) {
            throw Exception(At_Token(tok, scanner),
                stringify("unexpected token when expecting ", what));
        } else {
            scanner.push_token(tok);
            return nullptr;
        }
    }
}

}
