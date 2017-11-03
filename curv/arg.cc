// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <cmath>
#include <curv/arg.h>
#include <curv/exception.h>
#include <curv/phrase.h>

namespace curv {

void At_Arg::get_locations(std::list<Location>& locs) const
{
    assert(eval_frame_.call_phrase_ != nullptr);
    auto arg = eval_frame_.call_phrase_->arg_;
    locs.push_back(arg->location());

    // We only dump the stack starting at the parent call frame,
    // for cosmetic reasons. It looks stupid to underline one of the
    // arguments in a function call, and on the next line,
    // underline the same entire function call.
    get_frame_locations(eval_frame_.parent_frame_, locs);

}

Shared<const String>
At_Arg::rewrite_message(Shared<const String> msg) const
{
    if (arg_index_ < 0)
        return msg;
    else
        return stringify("at argument[",arg_index_,"], ", msg);
}

// TODO: Most of the following functions are redundant with the Value API.

bool arg_to_bool(Value val, const Context& ctx)
{
    if (!val.is_bool())
        throw Exception(ctx, "not boolean");
    return val.get_bool_unsafe();
}

auto arg_to_list(Value val, const Context& ctx)
-> List&
{
    if (!val.is_ref())
        throw Exception(ctx, "not a list");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_list)
        throw Exception(ctx, "not a list");
    return (List&)ref;
}

int arg_to_int(Value val, int lo, int hi, const Context& ctx)
{
    if (!val.is_num())
        throw Exception(ctx, "not an integer");
    double num = val.get_num_unsafe();
    double intf;
    double frac = modf(num, &intf);
    if (frac != 0.0)
        throw Exception(ctx, "not an integer");
    if (intf < (double)lo || intf > (double)hi)
        throw Exception(ctx, stringify("not in range [",lo,"..",hi,"]"));
    return (int)intf;
}

auto arg_to_num(Value val, const Context& ctx)
-> double
{
    if (!val.is_num())
        throw Exception(ctx, "not a number");
    return val.get_num_unsafe();
}

} // namespace curv
