// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cmath>
#include <curv/arg.h>
#include <curv/exception.h>

auto curv::get_arg(const Phrase& argsource, size_t i)
-> const Phrase&
{
    auto parens = dynamic_cast<const Paren_Phrase*>(&argsource);
    if (parens != nullptr) {
        assert(i < parens->args_.size());
        return *parens->args_[i].expr_;
    }
    assert(i < 1);
    return argsource;
}

auto curv::arg_to_list(Value val, const Phrase& source)
-> List&
{
    if (!val.is_ref())
        throw Phrase_Error(source, "not a list");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_list)
        throw Phrase_Error(source, "not a list");
    return (List&)ref;
}

auto curv::arg_to_string(Value val, const Phrase& source)
-> String&
{
    if (!val.is_ref())
        throw Phrase_Error(source, "not a string");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_string)
        throw Phrase_Error(source, "not a string");
    return (String&)ref;
}

int curv::arg_to_int(Value val, int lo, int hi, const Phrase& source)
{
    if (!val.is_num())
        throw Phrase_Error(source, "not an integer");
    double num = val.get_num_unsafe();
    double intf;
    double frac = modf(num, &intf);
    if (frac != 0.0)
        throw Phrase_Error(source, "not an integer");
    if (intf < (double)lo || intf > (double)hi)
        throw Phrase_Error(source, stringify("not in range [",lo,"..",hi,"]"));
    return (int)intf;
}
