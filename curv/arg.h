// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_ARG_H
#define CURV_ARG_H

#include <curv/list.h>
#include <curv/string.h>
#include <curv/record.h>
#include <curv/context.h>

namespace curv {

bool arg_to_bool(Value, const Context&);
List& arg_to_list(Value, const Context&);
int arg_to_int(Value, int, int, const Context&);
double arg_to_num(Value, const Context&);

/// The exception context for the i'th argument in a function call.
struct At_Arg : public Context
{
    int arg_index_;
    Frame& eval_frame_;

    At_Arg(int i, Frame& f) : arg_index_(i), eval_frame_(f) {}
    At_Arg(Frame& f) : arg_index_(-1), eval_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
    Shared<const String> rewrite_message(Shared<const String>) const override;
};

} // namespace curv
#endif // header guard
