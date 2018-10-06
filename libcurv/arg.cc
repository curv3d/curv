// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <cmath>
#include <libcurv/arg.h>
#include <libcurv/exception.h>
#include <libcurv/phrase.h>

namespace curv {

// TODO:
// * Merge with Value API?
// * arg_to_list is unsafe, should return Shared<List>.

auto arg_to_list(Value val, const Context& ctx)
-> List&
{
    if (val.is_ref()) {
        Ref_Value& ref( val.get_ref_unsafe() );
        if (ref.type_ == Ref_Value::ty_list)
            return (List&)ref;
    }
    throw Exception(ctx, stringify("is not a list: ",val));
}

} // namespace curv
