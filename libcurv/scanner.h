// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SCANNER_H
#define LIBCURV_SCANNER_H

#include <vector>
#include <libcurv/source.h>
#include <libcurv/token.h>
#include <libcurv/sstate.h>

namespace curv {

struct Scanner_Opts
{
    unsigned skip_prefix_ = 0;
    Scanner_Opts& skip_prefix(unsigned n) { skip_prefix_=n; return *this; }
};

/// \brief A lexical analyser.
///
/// The state of a lexical analyser is stored in this class.
/// get_token() gets the next token.
/// push_token() pushes back a previously got token,
/// supporting infinite lookahead.
struct Scanner
{
    Shared<const Source> source_;
    Source_State& sstate_;
    Token string_begin_;
    const char* ptr_;
    std::vector<Token> lookahead_;

    Scanner(Shared<const Source> s, Source_State& ss, Scanner_Opts opts = {})
    :
        source_(std::move(s)),
        sstate_(ss),
        string_begin_(),
        ptr_(source_->begin() + opts.skip_prefix_),
        lookahead_()
    {}
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
