// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/arg.h>
#include <curv/exception.h>

auto curv::arg_to_list(Value val, const char* name)
-> List&
{
    if (!val.is_ref())
        throw Arg_Error(name, "not a list");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_list)
        throw Arg_Error(name, "not a list");
    return (List&)ref;
}
