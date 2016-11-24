// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cmath>
#include <curv/arg.h>
#include <curv/exception.h>
#include <curv/phrase.h>

void curv::At_Arg::get_locations(std::list<Location>& locs) const
{
    assert(eval_frame_ != nullptr);
    assert(eval_frame_->call_phrase != nullptr);
    auto call_phrase =
        dynamic_cast<const Call_Phrase*>(eval_frame_->call_phrase);
    assert(call_phrase != nullptr);

    const Phrase& arg = call_phrase->at(arg_index_);

    locs.push_back(arg.location());
    // We only dump the stack starting at the parent call frame,
    // for cosmetic reasons. It looks stupid to underline one of the
    // arguments in a function call, and on the next line,
    // underline the same entire function call.
    get_frame_locations(eval_frame_->parent_frame, locs);
}

auto curv::arg_to_list(Value val, const Context& ctx)
-> List&
{
    if (!val.is_ref())
        throw Exception(ctx, "not a list");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_list)
        throw Exception(ctx, "not a list");
    return (List&)ref;
}

auto curv::arg_to_string(Value val, const Context& ctx)
-> String&
{
    if (!val.is_ref())
        throw Exception(ctx, "not a string");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_string)
        throw Exception(ctx, "not a string");
    return (String&)ref;
}

int curv::arg_to_int(Value val, int lo, int hi, const Context& ctx)
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
