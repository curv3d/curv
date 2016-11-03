// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SCANNER_H
#define CURV_SCANNER_H

#include <vector>
#include <curv/script.h>
#include <curv/token.h>
#include <curv/exception.h>
#include <curv/frame.h>
#include <curv/context.h>

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
private:
    const char* ptr_;
    std::vector<Token> lookahead_;
public:
    Scanner(const Script&, Frame*);
    Token get_token();
    void push_token(Token);
};

struct At_Token : public Context
{
    Token tok_;
    const Scanner& scanner_;

    At_Token(Token tok, const Scanner& scanner)
    : tok_(std::move(tok)), scanner_(scanner)
    {}

    virtual void get_locations(std::list<Location>& locs) const
    {
        locs.emplace_back(scanner_.script_, tok_);
        get_frame_locations(scanner_.eval_frame_, locs);
    }
};

} // namespace curv
#endif // header guard
