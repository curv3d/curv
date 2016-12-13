// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ARG_H
#define CURV_ARG_H

#include <curv/list.h>
#include <curv/string.h>
#include <curv/record.h>
#include <curv/context.h>

namespace curv {

List& arg_to_list(Value, const Context&);
String& arg_to_string(Value, const Context&);
int arg_to_int(Value, int, int, const Context&);
Record& arg_to_record(Value, const Context&);

/// The exception context for the i'th argument in a function call.
struct At_Arg : public Context
{
    size_t arg_index_;
    Frame* eval_frame_;

    At_Arg(size_t i, Frame* f) : arg_index_(i), eval_frame_(f) {}

    void get_locations(std::list<Location>& locs) const override;
};

} // namespace curv
#endif // header guard
