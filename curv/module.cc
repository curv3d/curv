// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/module.h>
#include <curv/function.h>

void
curv::Module::print(std::ostream& out) const
{
    out << "{";
    for (auto f : *this) {
        out << f.first << "=";
        f.second.print(out);
        out << ";";
    }
    for (auto e : *elements_) {
        out << e << ";";
    }
    out << "}";
}

curv::Value
curv::Module::get(size_t i) const
{
    Value val = (*slots_)[i];
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        if (ref.type_ == Ref_Value::ty_lambda)
            return {make<Closure>((Lambda&)ref, *slots_)};
    }
    return val;
}
