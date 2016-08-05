// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SCANNER_H
#define CURV_SCANNER_H

#include <vector>
#include <curv/script.h>
#include <curv/token.h>

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
private:
    const char* ptr_;
    std::vector<Token> lookahead_;
public:
    Scanner(const Script&);
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
