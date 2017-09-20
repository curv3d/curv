// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SCANNER_H
#define CURV_SCANNER_H

#include <vector>
#include <curv/script.h>
#include <curv/token.h>
#include <curv/frame.h>

namespace curv {

/// \brief A lexical analyzer.
///
/// The state of a lexical analyzer is stored in this class.
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
