// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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

    Token tok;
    const char* p = ptr_;
    const char* first = script_.first;
    const char* last = script_.last;

    // collect whitespace and comments
    tok.first_white = p - first;
    while (p < last && isspace(*p))
        ++p;
    tok.first = p - first;

    // recognize end of script
    if (p == last) {
        tok.kind = Token::k_end;
        goto success;
    }

    // Recognize a numeral. Compatible with C and strtod().
    //   numeral ::= significand exponent?
    //   significand ::= digits | digits "." | "." digits | digits "." digits
    //   exponent ::= "e" sign? digits
    //   sign ::= "+" | "-"
    //   digits ::= /[0-9]+/
    if (isdigit(*p) || (*p == '.' && p+1 < last && isdigit(p[1]))) {
        while (p < last && isdigit(*p))
            ++p;
        if (p < last && *p == '.') {
            ++p;
            while (p < last && isdigit(*p))
                ++p;
        }
        if (p < last && (*p == 'e' || *p == 'E')) {
            ++p;
            if (p < last && (*p == '+' || *p == '-'))
                ++p;
            if (p == last || !isdigit(*p)) {
                while (p < last && (isalnum(*p) || *p == '_'))
                    ++p;
                tok.last = p - first;
                ptr_ = p;
                throw Token_Error(script_, tok, "bad numeral");
            }
            while (p < last && isdigit(*p))
                ++p;
        }
        if (p < last && (isalpha(*p) || *p == '_')) {
            while (p < last && (isalnum(*p) || *p == '_'))
                ++p;
            tok.last = p - first;
            ptr_ = p;
            throw Token_Error(script_, tok, "bad numeral");
        }
        tok.kind = Token::k_num;
        goto success;
    }

    // recognize an identifier
    if (isalpha(*p) || *p == '_') {
        while (p < last && (isalnum(*p) || *p == '_'))
            ++p;
        aux::Range<const char*> id(first+tok.first, p);
        if (id == "if")
            tok.kind = Token::k_if;
        else if (id == "else")
            tok.kind = Token::k_else;
        else if (id == "let")
            tok.kind = Token::k_let;
        else
            tok.kind = Token::k_ident;
        goto success;
    }

    // recognize remaining tokens
    switch (*p++) {
    case '(':
        tok.kind = Token::k_lparen;
        goto success;
    case ')':
        tok.kind = Token::k_rparen;
        goto success;
    case '[':
        tok.kind = Token::k_lbracket;
        goto success;
    case ']':
        tok.kind = Token::k_rbracket;
        goto success;
    case '{':
        tok.kind = Token::k_lbrace;
        goto success;
    case '}':
        tok.kind = Token::k_rbrace;
        goto success;
    case '.':
        tok.kind = Token::k_dot;
        goto success;
    case ',':
        tok.kind = Token::k_comma;
        goto success;
    case ';':
        tok.kind = Token::k_semicolon;
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
    case '^':
        tok.kind = Token::k_power;
        goto success;
    case '@':
        tok.kind = Token::k_at;
        goto success;
    case '=':
        if (p < last && *p == '=') {
            tok.kind = Token::k_equal;
            ++p;
        } else
            tok.kind = Token::k_equate;
        goto success;
    case '!':
        if (p < last && *p == '=') {
            tok.kind = Token::k_not_equal;
            ++p;
        } else
            tok.kind = Token::k_not;
        goto success;
    case '<':
        if (p < last && *p == '=') {
            tok.kind = Token::k_less_or_equal;
            ++p;
        } else
            tok.kind = Token::k_less;
        goto success;
    case '>':
        if (p < last && *p == '=') {
            tok.kind = Token::k_greater_or_equal;
            ++p;
        } else
            tok.kind = Token::k_greater;
        goto success;
    case '&':
        if (p < last && *p == '&') {
            tok.kind = Token::k_and;
            ++p;
        } else
            goto error;
        goto success;
    case '|':
        if (p < last && *p == '|') {
            tok.kind = Token::k_or;
            ++p;
        } else
            goto error;
        goto success;
    case '"':
        for (;;) {
            if (p == last) {
                tok.last = p - first;
                tok.kind = Token::k_bad_token;
                ptr_ = p;
                throw Token_Error(script_, tok, "unterminated string literal");
            }
            if (*p == '"') {
                ++p;
                tok.kind = Token::k_string;
                goto success;
            }
            ++p;
        }
    }

    // report an error
error:
    tok.last = p - first;
    tok.kind = Token::k_bad_token;
    ptr_ = p;
    throw Char_Error(script_, tok);

success:
    tok.last = p - first;
    ptr_ = p;
    //cerr << "get_token fresh " << tok << "\n";
    return tok;
}
