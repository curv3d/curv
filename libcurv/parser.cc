// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

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

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/die.h>
#include <libcurv/parser.h>
#include <libcurv/scanner.h>

using namespace std;

namespace curv {

Shared<Phrase> parse_list(Scanner&);
Shared<Phrase> parse_semicolons(Scanner&, Shared<Phrase> firstitem);
Shared<Phrase> parse_commas(Scanner&, Shared<Phrase> firstitem);
Shared<Phrase> parse_item(Scanner&);
Shared<Phrase> parse_ritem(Scanner&);
Shared<Phrase> parse_pipeline(Scanner&);
Shared<Phrase> parse_disjunction(Scanner&);
Shared<Phrase> parse_conjunction(Scanner&);
Shared<Phrase> parse_relation(Scanner&);
Shared<Phrase> parse_sum(Scanner&);
Shared<Phrase> parse_product(Scanner&);
Shared<Phrase> parse_power(Scanner&);
Shared<Phrase> parse_postfix(Scanner&);
Shared<Phrase> parse_primary(Scanner&, const char* what);

// Parse a program (source file or CLI command), return a syntax tree.
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
        return make<Empty_Phrase>(Location{scanner.source_, begin});
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
        firstitem = make<Empty_Phrase>(Location{scanner.source_, empty});
    }
    auto semis = make<Semicolon_Phrase>();
    semis->args_.push_back({firstitem, tok});
    for (;;) {
        tok = scanner.get_token();
        if (is_list_end_token(tok.kind_)) {
            scanner.push_token(tok);
            Token etok = tok;
            etok.last_ = etok.first_;
            auto empty = make<Empty_Phrase>(Location{scanner.source_, etok});
            semis->args_.push_back({empty, etok});
            return semis;
        }
        if (tok.kind_ == Token::k_semicolon) {
            Token etok = tok;
            etok.last_ = etok.first_;
            auto empty = make<Empty_Phrase>(Location{scanner.source_, etok});
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

// item : ritem | ritem `where` ritem
Shared<Phrase>
parse_item(Scanner& scanner)
{
    auto ritem = parse_ritem(scanner);
    Token tok = scanner.get_token();
    if (tok.kind_ == Token::k_where) {
        return make<Where_Phrase>(std::move(ritem), tok, parse_ritem(scanner));
    } else {
        scanner.push_token(tok);
        return ritem;
    }
}

bool
is_ritem_end_token(Token::Kind k)
{
    if (is_list_end_token(k))
        return true;
    switch (k) {
    case Token::k_comma:
    case Token::k_semicolon:
    case Token::k_where:
    case Token::k_else:
        return true;
    default:
        return false;
    }
}

// Low precedence right associative operators.
//
// ritem : pipeline
//  | ... ritem
//  | 'include' ritem
//  | 'local' ritem
//  | pipeline = ritem
//  | pipeline := ritem
//  | pipeline :
//  | pipeline : ritem
//  | pipeline -> ritem
//  | pipeline << ritem
//  | 'if' primary ritem
//  | 'if' primary ritem 'else' ritem
//  | 'for' '(' ritem 'in' ritem ')' ritem
//  | 'while' parens ritem
//  | 'do' list 'in' ritem
//  | 'let' list 'in' ritem
//  | 'parametric' list 'in' item
Shared<Phrase>
parse_ritem(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_ellipsis:
    case Token::k_include:
        return make<Unary_Phrase>(tok, parse_ritem(scanner));
    case Token::k_local:
        return make<Local_Phrase>(tok, parse_ritem(scanner));
    case Token::k_if:
      {
        auto condition = parse_primary(scanner, "condition following 'if'");
        auto then_expr = parse_ritem(scanner);
        Token tok2 = scanner.get_token();
        if (tok2.kind_ != Token::k_else) {
            scanner.push_token(tok2);
            return make<If_Phrase>(
                tok, condition, then_expr, Token{}, nullptr);
        }
        auto else_expr = parse_ritem(scanner);
        return make<If_Phrase>(
            tok, condition, then_expr, tok2, else_expr);
      }
    case Token::k_do:
    case Token::k_let:
    case Token::k_parametric:
      {
        auto bindings = parse_list(scanner);
        Token tok2 = scanner.get_token();
        if (tok2.kind_ != Token::k_in)
            throw Exception(At_Token(tok2, scanner),
                "syntax error: expecting 'in'");
        // for parametric, call parse_item, not parse_ritem.
        // 'parametric params in ... where (bindings)' is parsed as
        // 'parametric params in (... where (bindings))'.
        Shared<Phrase> body;
        if (tok.kind_ == Token::k_parametric)
            body = parse_item(scanner);
        else
            body = parse_ritem(scanner);
        return make<Let_Phrase>(tok, bindings, tok2, body);
      }
    case Token::k_for:
      {
        Token tok2 = scanner.get_token();
        if (tok2.kind_ != Token::k_lparen) {
            throw Exception(At_Token(tok2, scanner),
                "syntax error: expecting '(' after 'for'");
        }

        auto pat = parse_primary(scanner, "'for' pattern");

        Token tok3 = scanner.get_token();
        if (tok3.kind_ != Token::k_in) {
            throw Exception(At_Token(tok3, scanner),
                "syntax error: expecting 'in'");
        }

        auto listexpr = parse_ritem(scanner);

        Token tok4 = scanner.get_token();
        if (tok4.kind_ != Token::k_rparen) {
            throw Exception(At_Token(tok4, scanner),
                "syntax error: expecting ')'");
        }

        auto body = parse_ritem(scanner);

        return make<For_Phrase>(tok, tok2, pat, tok3, listexpr, tok4, body);
      }
    case Token::k_while:
      {
        auto p = parse_primary(scanner, "argument following 'while'");
        auto args = cast<Paren_Phrase>(p);
        if (args == nullptr)
            throw Exception(At_Phrase(*p, scanner),
                "while: malformed argument");
        auto body = parse_ritem(scanner);
        return make<While_Phrase>(tok, args, body);
      }
    default:
        break;
    }

    scanner.push_token(tok);
    auto left = parse_pipeline(scanner);
    tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_equate:
        // NOTE: calling parse_item, not parse_ritem.
        // 'param = body where (bindings)' is parsed as
        // 'param = (body where (bindings))'.
        // TODO: change the parse (calling parse_ritem) so that subexpressions
        // within the 'param' are in the scope of the while.
        return make<Recursive_Definition_Phrase>(
            std::move(left), tok, parse_item(scanner));
    case Token::k_assign:
        if (auto unary = cast<Unary_Phrase>(left)) {
            if (unary->op_.kind_ == Token::k_var) {
                return make<Sequential_Definition_Phrase>(
                    unary->op_, std::move(unary->arg_), tok,
                    parse_ritem(scanner));
            }
        }
        return make<Assignment_Phrase>(
            std::move(left), tok, parse_ritem(scanner));
    case Token::k_colon:
      {
        auto tok2 = scanner.get_token();
        scanner.push_token(tok2);
        Shared<Phrase> right;
        if (is_ritem_end_token(tok2.kind_)) {
            tok2.kind_ = Token::k_missing;
            tok2.last_ = tok2.first_;
            right = make<Empty_Phrase>(Location{scanner.source_, tok2});
        } else {
            right = parse_ritem(scanner);
        }
        return make<Binary_Phrase>(std::move(left), tok, std::move(right));
      }
    case Token::k_right_arrow:
        return make<Lambda_Phrase>(
            std::move(left), tok, parse_ritem(scanner));
    case Token::k_left_call:
        return make<Call_Phrase>(
            std::move(left), parse_ritem(scanner), tok);
    default:
        scanner.push_token(tok);
        return left;
    }
}

// pipeline : disjunction
//  | pipeline >> disjunction
//  | pipeline ` postfix ` disjunction
//  | pipeline :: disjunction
Shared<Phrase>
parse_pipeline(Scanner& scanner)
{
    auto left = parse_disjunction(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_right_call:
            left = make<Call_Phrase>(
                parse_disjunction(scanner),
                std::move(left),
                tok);
            continue;
        case Token::k_colon_colon:
            left = make<Predicate_Assertion_Phrase>(
                std::move(left), tok, parse_disjunction(scanner));
            continue;
        case Token::k_backtick:
          {
            auto postfix = parse_postfix(scanner);
            Token tok2 = scanner.get_token();
            if (tok2.kind_ != Token::k_backtick) {
                throw Exception(At_Token(tok2, scanner),
                    "syntax error: expecting closing backtick (`)");
            }
            auto right = parse_disjunction(scanner);
            auto pair = make<Comma_Phrase>();
            pair->args_.push_back({left, {}});
            pair->args_.push_back({right, {}});
            auto arg = make<Paren_Phrase>(Token{}, pair, Token{});
            left = make<Call_Phrase>(postfix, arg, tok, tok2);
            continue;
          }
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// disjunction : conjunction | disjunction || conjunction
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

// relation : sum
//  | sum == sum | sum != sum
//  | sum < sum | sum > sum
//  | sum <= sum | sum >= sum
//  | sum .. sum
//  | sum .. sum `by` sum
//  | sum ..< sum
//  | sum ..< sum `by` sum
Shared<Phrase>
parse_relation(Scanner& scanner)
{
    auto left = parse_sum(scanner);
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_equal:
    case Token::k_not_equal:
    case Token::k_less:
    case Token::k_less_or_equal:
    case Token::k_greater:
    case Token::k_greater_or_equal:
        return make<Binary_Phrase>(
            std::move(left), tok, parse_sum(scanner));
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

// product : power | product * power | product / power
Shared<Phrase>
parse_product(Scanner& scanner)
{
    auto left = parse_power(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_times:
        case Token::k_over:
            left = make<Binary_Phrase>(
                std::move(left), tok, parse_power(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// High precedence, right associative infix and prefix operators.
//
// power
//  : postfix
//  | - power
//  | + power
//  | ! power
//  | 'var' power
//  | postfix ^ power
Shared<Phrase>
parse_power(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_plus:
    case Token::k_minus:
    case Token::k_not:
    case Token::k_var:
        return make<Unary_Phrase>(tok, parse_power(scanner));
    default:
        scanner.push_token(tok);
        break;
    }

    auto postfix = parse_postfix(scanner);
    tok = scanner.get_token();
    switch (tok.kind_) {
    case Token::k_power:
        return make<Binary_Phrase>(postfix, tok, parse_power(scanner));
    default:
        scanner.push_token(tok);
        return postfix;
    }
}

// postfix : primary
//  | postfix primary
//  | postfix . primary
Shared<Phrase>
parse_postfix(Scanner& scanner)
{
    auto postfix = parse_primary(scanner, "expression");
    Token tok;
    for (;;) {
        tok = scanner.get_token();
        switch (tok.kind_) {
        case Token::k_dot:
            postfix = make<Dot_Phrase>(postfix, tok,
                parse_primary(scanner, "expression following ."));
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
                segments, *scanner.source_, begin, tok);
        case Token::k_string_segment:
            segments.push_back(make<String_Segment_Phrase>(*scanner.source_, tok));
            continue;
        case Token::k_string_newline:
        case Token::k_char_escape:
            segments.push_back(make<Char_Escape_Phrase>(*scanner.source_, tok));
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
        case Token::k_dollar_bracket:
          {
            auto state = scanner.string_begin_;
            scanner.string_begin_.kind_ = Token::k_missing;
            auto brackets =
                parse_delimited<Bracket_Phrase>(tok, Token::k_rbracket, scanner);
            scanner.string_begin_ = state;
            segments.push_back(make<Bracket_Segment_Phrase>(brackets));
            continue;
          }
        case Token::k_dollar_brace:
          {
            auto state = scanner.string_begin_;
            scanner.string_begin_.kind_ = Token::k_missing;
            auto braces =
                parse_delimited<Bracket_Phrase>(tok, Token::k_rbrace, scanner);
            scanner.string_begin_ = state;
            segments.push_back(make<Brace_Segment_Phrase>(braces));
            continue;
          }
        case Token::k_dollar_ident:
          {
            Token id = tok;
            ++id.first_;
            auto ident = make<Identifier>(*scanner.source_, id);
            segments.push_back(make<Ident_Segment_Phrase>(ident));
            continue;
          }
        default:
            die("parse_string: bad string segment token type");
        }
    }
}

// primary : symbol | numeral | identifier | string | parens | brackets | braces
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
    case Token::k_hexnum:
    case Token::k_symbol:
        return make<Numeral>(*scanner.source_, tok);
    case Token::k_ident:
        return make<Identifier>(*scanner.source_, tok);
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
