// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/vec.h>

#include <libcurv/context.h>
#include <libcurv/list.h>

namespace curv {

bool unbox_vec2(Value val, Vec2& out)
{
    auto list = val.maybe<List>();
    if (list) {
        if (list->size() == 2) {
            double e0 = list->at(0).to_num_or_nan();
            double e1 = list->at(1).to_num_or_nan();
            if (e0 == e0 && e1 == e1) {
                out.x = e0;
                out.y = e1;
                return true;
            }
        }
    }
    return false;
}

Vec3 value_to_vec3(Value val, const Context& cx)
{
    auto list = val.to<List>(cx);
    list->assert_size(3, cx);
    Vec3 v;
    v.x = list->at(0).to_num(At_Index(0, cx));
    v.y = list->at(1).to_num(At_Index(1, cx));
    v.z = list->at(2).to_num(At_Index(2, cx));
    return v;
}

} // namespace
