// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/function.h>
#include <curv/exception.h>
#include <curv/context.h>

using namespace curv;

void
Function::print(std::ostream& out) const
{
    out << "<function>";
}

GL_Value
Function::gl_call(GL_Args&, GL_Compiler&) const
{
    throw Exception({}, "the Geometry Compiler is not implemented");
}

Value
Closure::call(Frame& f)
{
    f.nonlocal = &*nonlocal_;
    return expr_->eval(f);
}

void
Lambda::print(std::ostream& out) const
{
    out << "<lambda>";
}
