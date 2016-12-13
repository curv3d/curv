// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/function.h>

void
curv::Function::print(std::ostream& out) const
{
    out << "<function>";
}

curv::Value
curv::Closure::call(Frame& f)
{
    f.nonlocal = &*nonlocal_;
    return expr_->eval(f);
}

void
curv::Lambda::print(std::ostream& out) const
{
    out << "<lambda>";
}
