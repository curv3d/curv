// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ARG_H
#define LIBCURV_ARG_H

#include <libcurv/list.h>
#include <libcurv/string.h>
#include <libcurv/record.h>
#include <libcurv/context.h>

namespace curv {

bool arg_to_bool(Value, const Context&);
List& arg_to_list(Value, const Context&);
int arg_to_int(Value, int, int, const Context&);
double arg_to_num(Value, const Context&);

/// The exception context for the i'th argument in a function call.
struct At_Arg : public Context
{
    Frame& eval_frame_;

    At_Arg(Frame& f) : eval_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
};

} // namespace curv
#endif // header guard
