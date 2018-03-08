// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_SCANNER_H
#define CURV_SCANNER_H

#include <vector>
#include <curv/script.h>
#include <curv/token.h>
#include <curv/frame.h>

namespace curv {

/// \brief A lexical analyser.
///
/// The state of a lexical analyser is stored in this class.
/// get_token() gets the next token.
/// push_token() pushes back a previously got token,
/// supporting infinite lookahead.
struct Scanner
{
    const Script& script_;
    Frame* eval_frame_;
    Token string_begin_;
private:
    const char* ptr_;
    std::vector<Token> lookahead_;
public:
    Scanner(const Script&, Frame*);
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
