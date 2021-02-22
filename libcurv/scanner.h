// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SCANNER_H
#define LIBCURV_SCANNER_H

#include <vector>
#include <libcurv/source.h>
#include <libcurv/token.h>
#include <libcurv/frame.h>

namespace curv {

struct Scanner_Opts
{
    Frame* file_frame_ = nullptr;
    Scanner_Opts& file_frame(Frame* f) { file_frame_=f; return *this; }

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
    System& system_;
    /// file_frame_ is nullptr, unless we are scanning a source file due to
    /// an evaluation-time call to `file`. It's used by the Exception Context,
    /// to add a stack trace to compile time errors.
    Frame* file_frame_;
    Token string_begin_;
    const char* ptr_;
    std::vector<Token> lookahead_;

    Scanner(Shared<const Source> s, System& system, Scanner_Opts opts = {})
    :
        source_(move(s)),
        system_(system),
        file_frame_(opts.file_frame_),
        string_begin_(),
        ptr_(source_->begin() + opts.skip_prefix_),
        lookahead_()
    {}
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
