// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/scanner.h>
#include <curv/exception.h>

using namespace curv;
using namespace std;

Scanner::Scanner(const Script& s)
:
    script_(s),
    ptr_(s.begin()),
    lookahead_()
{
}

void
Scanner::push_token(Token tok)
{
    //cerr << "push_token " << tok << "\n";
    lookahead_.push_back(tok);
}

Token
Scanner::get_token()
{
    if (!lookahead_.empty()) {
        auto tok = lookahead_.back();
        lookahead_.pop_back();
        //cerr << "get_token from lookahead" << tok << "\n";
        return tok;
    }

    Token tok(script_);
    const char* p = ptr_;
    const char* first = script_.first;
    const char* last = script_.last;

    // collect whitespace and comments
    tok.white_first = p - first;
    while (p < last && isspace(*p))
        ++p;
    tok.first = p - first;

    // recognize end of script
    if (p == last) {
        tok.kind = Token::k_end;
        goto success;
    }

    // recognize a numeral
    if (isdigit(*p)) {
        while (p < last && isdigit(*p))
            ++p;
        tok.kind = Token::k_num;
        goto success;
    }

    // recognize single-character tokens
    switch (*p++) {
    case '(':
        tok.kind = Token::k_lparen;
        goto success;
    case ')':
        tok.kind = Token::k_rparen;
        goto success;
    case '+':
        tok.kind = Token::k_plus;
        goto success;
    case '-':
        tok.kind = Token::k_minus;
        goto success;
    case '*':
        tok.kind = Token::k_times;
        goto success;
    case '/':
        tok.kind = Token::k_over;
        goto success;
    }
    --p;

    // report an error
    throw Exception("illegal character");

success:
    tok.last = p - first;
    ptr_ = p;
    //cerr << "get_token fresh " << tok << "\n";
    return tok;
}
