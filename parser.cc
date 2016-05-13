#include <iostream>
#include <cstdlib>
#include <curv/parse.h>
#include <curv/scanner.h>

using namespace std;
using namespace curv;

namespace curv {

unique_ptr<Expr> parse_sum(Scanner&);
unique_ptr<Expr> parse_product(Scanner&);
unique_ptr<Expr> parse_unary(Scanner&);
unique_ptr<Expr> parse_atom(Scanner&);
void report_syntax_error(Token tok, const char*);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
/*
sum : product | sum + product | sum - product
product : unary | product * unary | product / unary
unary : atom | - unary | + unary
atom : numeral | ( sum )
*/
unique_ptr<Expr>
parse(const Script& script)
{
    Scanner scanner(script);
    auto expr = parse_sum(scanner);
    auto tok = scanner.get_token();
    if (tok.kind != Token::k_end)
        report_syntax_error(tok, "unexpected token at end of script");
    return expr;
}

// sum : product | sum + product | sum - product
unique_ptr<Expr>
parse_sum(Scanner& scanner)
{
    auto left = parse_product(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_plus:
        case Token::k_minus:
            left = make_unique<BinaryExpr>(
                tok, std::move(left), parse_product(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// product : unary | product * unary | product / unary
unique_ptr<Expr>
parse_product(Scanner& scanner)
{
    auto left = parse_unary(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_times:
        case Token::k_over:
            left = make_unique<BinaryExpr>(
                tok, std::move(left), parse_unary(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// unary : atom | - unary | + unary
unique_ptr<Expr>
parse_unary(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_plus:
    case Token::k_minus:
        return make_unique<UnaryExpr>(tok, parse_unary(scanner));
    default:
        scanner.push_token(tok);
        return parse_atom(scanner);
    }
}

// atom : numeral | ( sum )
unique_ptr<Expr>
parse_atom(Scanner& scanner)
{
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_num) {
        return make_unique<NumExpr>(tok);
    }
    if (tok.kind == Token::k_lparen) {
        auto expr = parse_sum(scanner);
        auto tok2 = scanner.get_token();
        if (tok2.kind == Token::k_rparen)
            return make_unique<ParenExpr>(tok,std::move(expr),tok2);
        report_syntax_error(tok2, "unexpected token when expecting ')'");
    }
    report_syntax_error(tok, "unexpected token when expecting atom");
}

void
report_syntax_error(Token tok, const char* msg)
{
    cerr << msg << " at " << tok.scriptname() << ":" << tok.lineno()
        << ": " << tok << "\n";
    exit(1);
}

}
