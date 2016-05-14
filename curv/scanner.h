/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#ifndef CURV_SCANNER_H
#define CURV_SCANNER_H

#include <vector>
#include <curv/token.h>

namespace curv {

/// \brief A lexical analyzer.
///
/// The state of a lexical analyzer is stored in this class.
/// get_token() gets the next token.
/// push_token() pushes back a previously got token,
/// supporting infinite lookahead.
class Scanner
{
private:
    const Script& script_;
    const char* ptr_;
    std::vector<Token> lookahead_;
public:
    Scanner(const Script&);
    Token get_token();
    void push_token(Token);
};

} // namespace curv
#endif // header guard
