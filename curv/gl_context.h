// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_GL_CONTEXT_H
#define CURV_GL_CONTEXT_H

#include <curv/context.h>
#include <curv/gl_compiler.h>

namespace curv {

struct At_GL_Frame : public Context
{
    GL_Frame* frame_;

    At_GL_Frame(GL_Frame* frame) : frame_(frame) {}

    virtual void get_locations(std::list<Location>& locs) const override;
};

void get_gl_frame_locations(const GL_Frame* f, std::list<Location>& locs);

/// Exception Context where we know the Phrase that contains the error.
struct At_GL_Phrase : public Context
{
    const Phrase& phrase_;
    GL_Frame* frame_;

    At_GL_Phrase(const Phrase& phrase, GL_Frame* frame);

    virtual void get_locations(std::list<Location>& locs) const override;
};

/// The exception context for the i'th argument in a function call.
struct At_GL_Arg : public Context
{
    size_t arg_index_;
    GL_Frame& eval_frame_;

    At_GL_Arg(size_t i, GL_Frame& f) : arg_index_(i), eval_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
};

} // namespace curv
#endif // header guard
