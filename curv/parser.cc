// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

// I'm currently using a hand coded recursive descent parser.
// * More flexible for trying out experimental syntax.
// * More flexible for error recovery and informative error messages.
// Alternatives are: Bison, Lemon, Boost.Spirit (maybe X3).
//
// However, I want a simple yaccable grammar, so once the language has
// stabilized, I might replace this with a Bison grammar.
//
// TODO: simple yaccable grammar, no conflicts or precedence declarations.
// TODO: remove dangling `else` ambiguity.
// TODO: $(expression) substitutions in string literals.
// TODO: infix `where` operator. Binds tighter than `->`.

#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/exception.h>
#include <curv/context.h>

using namespace std;
using namespace curv;
using namespace aux;

namespace curv {

Shared<Phrase> parse_list(Scanner&);
Shared<Phrase> parse_semicolons(Scanner&, Shared<Phrase> firstitem);
Shared<Phrase> parse_commas(Scanner&, Shared<Phrase> firstitem);
Shared<Phrase> parse_item(Scanner&);
Shared<Phrase> parse_disjunction(Scanner&);
Shared<Phrase> parse_conjunction(Scanner&);
Shared<Phrase> parse_relation(Scanner&);
Shared<Phrase> parse_range(Scanner&);
Shared<Phrase> parse_sum(Scanner&);
Shared<Phrase> parse_product(Scanner&);
Shared<Phrase> parse_unary(Scanner&);
Shared<Phrase> parse_postfix(Scanner&);
Shared<Phrase> parse_primary(Scanner&, const char* what);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
//
// program : list EOF
Shared<Program_Phrase>
parse_program(Scanner& scanner)
{
    auto list = parse_list(scanner);
    Token tok = scanner.get_token();
    if (tok.kind_ != Token::k_end)
        throw Exception(At_Token(tok, scanner), "syntax error in program");
    return make<Program_Phrase>(list, tok);
}

bool
is_list_end_token(Token::Kind k)
{
    switch (k) {
    case Token::k_end:
    case Token::k_rparen:
    case Token::k_rbracket:
    case Token::k_rbrace:
    case Token::k_in:
        return true;
    default:
        return false;
    }
}

// list : empty | item | commas | semicolons
Shared<Phrase>
parse_list(Scanner& scanner)
{
    auto tok = scanner.get_token();
    scanner.push_token(tok);
    if (is_list_end_token(tok.kind_)) {
        Token begin = tok;
        begin.last_ = begin.first_;
        return make<Empty_Phrase>(Location{scanner.script_, begin});
    }
    if (tok.kind_ == Token::k_semicolon)
        return parse_semicolons(scanner, nullptr);

    auto item = parse_item(scanner);
    tok = scanner.get_token();
    scanner.push_token(tok);
    if (is_list_end_token(tok.kind_))
        return item;
    if (tok.kind_ == Token::k_comma)
        return parse_commas(scanner, item);
    if (tok.kind_ == Token::k_semicolon)
        return parse_semicolons(scanner, item);
    throw Exception(At_Token(tok, scanner), "syntax error in list");
}

// commas : item `,` | item `,` item | item `,` commas
//
// When called, first item has been parsed, next token in scanner is comma.
Shared<Phrase>
parse_commas(Scanner& scanner, Shared<Phrase> firstitem)
{
    auto tok = scanner.get_token();
    assert(tok.kind_ == Token::k_comma);
    auto commas = make<Comma_Phrase>();
    commas->args_.push_back({firstitem, tok});

    for (;;) {
        // We previously saw: item `,`
        tok = scanner.get_token();
        if (is_list_end_token(tok.kind_)) {
            scanner.push_token(tok);
            return commas;
        }
        if (tok.kind_ == Token::k_comma) {
            throw Exception(At_Token(tok, scanner),
                "syntax error: two consecutive commas");
        }
        if (tok.kind_ == Token::k_semicolon) {
            throw Exception(At_Token(tok, scanner),
                "syntax error: can't mix commas and semicolons in same list");
        }
        scanner.push_token(tok);
        auto item = parse_item(scanner);
        tok = scanner.get_token();
        if (is_list_end_token(tok.kind_)) {
            commas->args_.push_back({item, {}});
            scanner.push_token(tok);
            return commas;
        }
        if (tok.kind_ == Token::k_comma) {
            commas->args_.push_back({item, tok});
            continue;
        }
        if (tok.kind_ == Token::k_semicolon) {
            throw Exception(At_Token(tok, scanner),
                "syntax error: can't mix commas and semicolons in same list");
        }
        throw Exception(At_Token(tok, scanner),
            "syntax error in comma-separated list");
    }
}

// semicolons : optitem | semicolons `;` optitem
// optitem : empty | item
//
// When called, first item has been parsed, next token in scanner is semicolon.
// If first item is empty, then firstitem==nullptr.
Shared<Phrase>
parse_semicolons(Scanner& scanner, Shared<Phrase> firstitem)
{
    auto tok = scanner.get_token();
    assert(tok.kind_ == Token::k_semicolon);
    if (firstitem == nullptr) {
        Token empty = tok;
        empty.last_ = empty.first_;
        firstitem = make<Empty_Phrase>(Location{scanner.script_, empty});
    }
    auto semis = make<Semicolon_Phrase>();
    semis->args_.push_back({firstitem, tok});
    for (;;) {
        tok = scanner.get_token();
        if (is_list_end_token(tok.kind_)) {
            scanner.push_token(tok);
            Token etok = tok;
            etok.last_ = etok.first_;
            auto empty = make<Empty_Phrase>(Location{scanner.script_, etok});
            semis->args_.push_back({empty, etok});
            return semis;
        }
        if (tok.kind_ == Token::k_semicolon) {
            Token etok = tok;
            etok.last_ = etok.first_;
            auto empty = make<Empty_Phrase>(Location{scanner.script_, etok});
            semis->args_.push_back({empty, tok});
            continue;
        }
        if (tok.kind_ == Token::k_comma) {
            throw Exception(At_Token(tok, scanner),
                "syntax error: can't mix commas and semicolons in same list");
        }
        scanner.push_token(tok);
        auto item = parse_item(scanner);
        tok = scanner.get_token();
        if (is_list_end_token(tok.kind_)) {
            scanner.push_token(tok);
            semis->args_.push_back({item, {}});
            return semis;
        }
        if (tok.kind_ == Token::k_semicolon) {
            semis->args_.push_back({item, tok});
            continue;
        }
        if (tok.kind_ == Token::k_comma) {
            throw Exception(At_Token(tok, scanner),
                "syntax error: can't mix commas and semicolons in same list");
        }
        throw Exception(At_Token(tok, scanner),
            "syntax error in semicolon-separated list");
    }
}

// Low precedence right associative operators.
//
// item : disjunction
//  | ... item
//  | disjunction = item
//  | disjunction := item
//  | disjunction : item
//  | disjunction -> item
//  | disjunction << item
//  | 'if' primary item
//  | 'if' primary item 'else' item
//  | 'for' '(' item 'in' item ')' item
//  | 'while' parens item
//  | 'let' list 'in' item
Shared<Phrase>
parse_item(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_ellipsis:
        return make<Unary_Phrase>(tok, parse_item(scanner));
    case Token::k_if:
      {
        auto condition = parse_primary(scanner, "condition following 'if'");
        auto then_expr = parse_item(scanner);
        Token tok2 = scanner.get_token();
        if (tok2.kind_ != Token::k_else) {
            scanner.push_token(tok2);
            return make<If_Phrase>(
                tok, condition, then_expr, Token{}, nullptr);
        }
        auto else_expr = parse_item(scanner);
        return make<If_Phrase>(
            tok, condition, then_expr, tok2, else_expr);
      }
    case Token::k_for:
      {
        Token tok2 = scanner.get_token();
        if (tok2.kind_ != Token::k_lparen) {
            throw Exception(At_Token(tok2, scanner),
                "syntax error: expecting '(' after 'for'");
        }

        auto p = parse_primary(scanner, "'for' iteration variable");
        auto id = cast<Identifier>(p);
        if (id == nullptr) {
            throw Exception(At_Phrase(*p, scanner.eval_frame_),
                "syntax error: expecting identifier after 'for('");
        }

        Token tok3 = scanner.get_token();
        if (tok3.kind_ != Token::k_in) {
            throw Exception(At_Token(tok3, scanner),
                "syntax error: expecting 'in'");
        }

        auto listexpr = parse_item(scanner);

        Token tok4 = scanner.get_token();
        if (tok4.kind_ != Token::k_rparen) {
            throw Exception(At_Token(tok4, scanner),
                "syntax error: expecting ')'");
        }

        auto body = parse_item(scanner);

        return make<For_Phrase>(tok, tok2, id, tok3, listexpr, tok4, body);
      }
    case Token::k_while:
      {
        auto p = parse_primary(scanner, "argument following 'while'");
        auto args = cast<Paren_Phrase>(p);
        if (args == nullptr)
            throw Exception(At_Phrase(*p, scanner.eval_frame_),
                "while: malformed argument");
        auto body = parse_item(scanner);
        return make<While_Phrase>(tok, args, body);
      }
    default:
        break;
    }

    scanner.push_token(tok);
    auto left = parse_disjunction(scanner);
    tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_equate:
        return make<Recursive_Definition_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_assign:
        if (auto unary = cast<Unary_Phrase>(left)) {
            if (unary->op_.kind_ == Token::k_var) {
                return make<Sequential_Definition_Phrase>(
                    unary->op_, std::move(unary->arg_), tok,
                    parse_item(scanner));
            }
        }
        return make<Assignment_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_colon:
        return make<Binary_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_in:
        return make<Binary_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_right_arrow:
        return make<Lambda_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_left_call:
        return make<Call_Phrase>(
            std::move(left), parse_item(scanner), tok);
    default:
        scanner.push_token(tok);
        return left;
    }
}

// disjunction : conjunction
//  | disjunction || conjunction
//  | disjunction >> conjunction
Shared<Phrase>
parse_disjunction(Scanner& scanner)
{
    auto left = parse_conjunction(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_or:
            left = make<Binary_Phrase>(
                std::move(left), tok, parse_conjunction(scanner));
            continue;
        case Token::k_right_call:
            left = make<Call_Phrase>(
                parse_conjunction(scanner),
                std::move(left),
                tok);
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
        switch (tok.kind_) {
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

// relation : range
//  | range == range | range != range
//  | range < range | range > range
//  | range <= range | range >= range
Shared<Phrase>
parse_relation(Scanner& scanner)
{
    auto left = parse_range(scanner);
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_equal:
    case Token::k_not_equal:
    case Token::k_less:
    case Token::k_less_or_equal:
    case Token::k_greater:
    case Token::k_greater_or_equal:
        return make<Binary_Phrase>(
            std::move(left), tok, parse_range(scanner));
    default:
        scanner.push_token(tok);
        return left;
    }
}

// range : sum
//  | sum .. sum
//  | sum .. sum `by` sum
//  | sum ..< sum
//  | sum ..< sum `by` sum
Shared<Phrase>
parse_range(Scanner& scanner)
{
    auto left = parse_sum(scanner);
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_range:
    case Token::k_open_range:
      {
        auto right = parse_sum(scanner);
        auto tok2 = scanner.get_token();
        Shared<Phrase> step;
        if (tok2.kind_ == Token::k_by) {
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
        switch (tok.kind_) {
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
        switch (tok.kind_) {
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

// unary : postfix | - unary | + unary | ! unary
Shared<Phrase>
parse_unary(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_plus:
    case Token::k_minus:
    case Token::k_not:
    case Token::k_var:
        return make<Unary_Phrase>(tok, parse_unary(scanner));
    default:
        scanner.push_token(tok);
        return parse_postfix(scanner);
    }
}

// postfix : primary
//  | postfix primary
//  | postfix . primary
//  | postfix ' primary
//  | postfix ^ unary
Shared<Phrase>
parse_postfix(Scanner& scanner)
{
    auto postfix = parse_primary(scanner, "expression");
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_power:
            return make<Binary_Phrase>(
                postfix, tok, parse_unary(scanner));
        case Token::k_dot:
        case Token::k_apostrophe:
            postfix = make<Binary_Phrase>(postfix, tok, parse_primary(scanner,
                tok.kind_ == Token::k_dot
                ? "expression following ."
                : "expression following '"));
            continue;
        default:
            scanner.push_token(tok);
            auto primary = parse_primary(scanner, nullptr);
            if (primary == nullptr)
                return postfix;
            postfix = make<Call_Phrase>(postfix, primary);
        }
    }
}

template<class Ph>
Shared<Phrase>
parse_delimited(Token& tok, Token::Kind close, Scanner& scanner)
{
    auto body = parse_list(scanner);
    auto tok2 = scanner.get_token();
    if (tok2.kind_ == Token::k_end)
        throw Exception(At_Token(tok, scanner), "unmatched delimiter");
    if (tok2.kind_ != close)
        throw Exception(At_Token(tok2, scanner), "syntax error in delimited phrase");
    return make<Ph>(tok, body, tok2);
}

Shared<Phrase>
parse_string(Scanner& scanner, Token begin)
{
    std::vector<Shared<const Segment_Phrase>> segments;
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_quote:
            return String_Phrase::make_elements(
                segments, scanner.script_, begin, tok);
        case Token::k_string_segment:
            segments.push_back(make<String_Segment_Phrase>(scanner.script_, tok));
            continue;
        case Token::k_char_escape:
            segments.push_back(make<Char_Escape_Phrase>(scanner.script_, tok));
            continue;
        case Token::k_dollar_paren:
          {
            auto state = scanner.string_begin_;
            scanner.string_begin_.kind_ = Token::k_missing;
            auto parens =
                parse_delimited<Paren_Phrase>(tok, Token::k_rparen, scanner);
            scanner.string_begin_ = state;
            segments.push_back(make<Paren_Segment_Phrase>(parens));
            continue;
          }
        case Token::k_dollar_brace:
          {
            auto state = scanner.string_begin_;
            scanner.string_begin_.kind_ = Token::k_missing;
            auto braces =
                parse_delimited<Paren_Phrase>(tok, Token::k_rbrace, scanner);
            scanner.string_begin_ = state;
            segments.push_back(make<Brace_Segment_Phrase>(braces));
            continue;
          }
        default:
            assert(0);
        }
    }
}

// primary : numeral | identifier | string | parens | brackets | braces
// parens : ( list )
// brackets : [ list ]
// braces : { list }
//
// If `what` is nullptr, then we are parsing an optional primary,
// and we return nullptr if no primary is found.
// Otherwise, `what` is used to construct an error message if a primary
// expression isn't found.
Shared<Phrase>
parse_primary(Scanner& scanner, const char* what)
{
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_num:
        return make<Numeral>(scanner.script_, tok);
    case Token::k_ident:
        return make<Identifier>(scanner.script_, tok);
    case Token::k_quote:
        return parse_string(scanner, tok);
    case Token::k_lparen:
        return parse_delimited<Paren_Phrase>(tok, Token::k_rparen, scanner);
    case Token::k_lbracket:
        return parse_delimited<Bracket_Phrase>(tok, Token::k_rbracket, scanner);
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

} // namespace curv
