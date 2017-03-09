// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

// TODO: Convert to use the re2c scanner generator. Then add UTF-8 support.

#include <curv/scanner.h>
#include <curv/exception.h>
#include <curv/context.h>

using namespace std;
namespace curv {

Scanner::Scanner(const Script& s, Frame* f)
:
    script_(s),
    eval_frame_(f),
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
    while (p < last) {
        if (isspace(*p)) {
            ++p;
        }
        else if (*p == '/' && p+1 < last) {
            if (p[1] == '/') {
                // A '//' comment continues to the end of the file or the line.
                p += 2;
                for (;;) {
                    if (p == last) break;
                    if (*p == '\n') {
                        ++p;
                        break;
                    }
                    ++p;
                }
            }
            else if (p[1] == '*') {
                // A '/*' comment continues to the first '*/', as in C.
                // An unterminated comment is an error.
                const char* begin_comment = p;
                p += 2;
                for (;;) {
                    if (p+1 < last && p[0]=='*' && p[1]=='/') {
                        p += 2;
                        break;
                    }
                    if (p == last) {
                        ptr_ = p;
                        tok.kind = Token::k_bad_token;
                        tok.first = begin_comment - first;
                        tok.last = last - first;
                        throw Exception(At_Token(tok, *this),
                            "unterminated comment");
                    }
                    ++p;
                }
            }
            else
                break;
        }
        else
            break;
    }
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
        if (p < last && *p == '.' && !(p+1 < last && p[1]=='.')) {
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
                throw Exception(At_Token(tok, *this), "bad numeral");
            }
            while (p < last && isdigit(*p))
                ++p;
        }
        if (p < last && (isalpha(*p) || *p == '_')) {
            while (p < last && (isalnum(*p) || *p == '_'))
                ++p;
            tok.last = p - first;
            ptr_ = p;
            throw Exception(At_Token(tok, *this), "bad numeral");
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
        else if (id == "letrec")
            tok.kind = Token::k_letrec;
        else if (id == "for")
            tok.kind = Token::k_for;
        else if (id == "by")
            tok.kind = Token::k_by;
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
        if (p < last && *p == '.') {
            if (p+1 < last && p[1] == '<') {
                tok.kind = Token::k_open_range;
                p += 2;
            } else if (p+1 < last && p[1] == '.') {
                tok.kind = Token::k_ellipsis;
                p += 2;
            } else {
                tok.kind = Token::k_range;
                ++p;
            }
        } else
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
        if (p < last && *p == '>') {
            tok.kind = Token::k_right_arrow;
            ++p;
        } else
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
    case '\'':
        tok.kind = Token::k_apostrophe;
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
        } else if (p < last && *p == '<') {
            tok.kind = Token::k_left_call;
            ++p;
        } else
            tok.kind = Token::k_less;
        goto success;
    case '>':
        if (p < last && *p == '=') {
            tok.kind = Token::k_greater_or_equal;
            ++p;
        } else if (p < last && *p == '>') {
            tok.kind = Token::k_right_call;
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
                throw Exception(At_Token(tok, *this),
                    "unterminated string literal");
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
    throw Exception(At_Token(tok, *this), illegal_character_message(p[-1]));

success:
    tok.last = p - first;
    ptr_ = p;
    //cerr << "get_token fresh " << tok << "\n";
    return tok;
}

} // namespace curv
