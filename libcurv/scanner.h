// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SCANNER_H
#define LIBCURV_SCANNER_H

#include <vector>
#include <libcurv/source.h>
#include <libcurv/token.h>
#include <libcurv/frame.h>

namespace curv {

/// \brief A lexical analyser.
///
/// The state of a lexical analyser is stored in this class.
/// get_token() gets the next token.
/// push_token() pushes back a previously got token,
/// supporting infinite lookahead.
struct Scanner
{
    const Source& source_;
    Frame* eval_frame_;
    Token string_begin_;
private:
    const char* ptr_;
    std::vector<Token> lookahead_;
public:
    Scanner(const Source&, Frame*);
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
