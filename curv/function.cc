// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/function.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/gl_context.h>

namespace curv {

void
Function::print(std::ostream& out) const
{
    out << "<function>";
}

GL_Value
Function::gl_call(GL_Frame& f) const
{
    throw Exception(At_GL_Frame(&f),
        "this function does not support the Geometry Compiler");
}

Value
Closure::call(Frame& f)
{
    f.nonlocal = &*nonlocal_;
    return expr_->eval(f);
}

GL_Value
Closure::gl_call(GL_Frame& f) const
{
    f.nonlocal = &*nonlocal_;
    return expr_->gl_eval(f);
}

void
Lambda::print(std::ostream& out) const
{
    out << "<lambda>";
}

} // namespace curv
