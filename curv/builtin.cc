// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <curv/arg.h>
#include <curv/builtin.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/function.h>
#include <boost/math/constants/constants.hpp>

using namespace curv;
using namespace std;
using namespace boost::math::double_constants;

Shared<Meaning>
Builtin_Value::to_meaning(const Identifier& id) const
{
    return curv::make_shared<Constant>(share<const Identifier>(id), value_);
}

/*
map["sqrt"] = {1, [](Frame& args) -> Value {
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw Exception(At_Arg(0, &args),
            stringify("sqrt(",args[0],"): domain error"));
}};
*/

Value
builtin_sqrt(Frame& args)
{
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw Exception(At_Arg(0, &args),
            stringify("sqrt(",args[0],"): domain error"));
}

Value
builtin_len(Frame& args)
{
    auto& list {arg_to_list(args[0], At_Arg(0, &args))};
    return {double(list.size())};
}

Value
builtin_file(Frame& args)
{
    // TODO: Pluggable file system abstraction, for unit testing and
    // abstracting the behaviour of `file` (would also support caching).
    // TODO: The builtin_namespace used by `file` is a pluggable parameter
    // at compile time.
    String& path {arg_to_string(args[0], At_Arg(0, &args))};
    return {eval_file(path, &args)};
}

const Namespace
curv::builtin_namespace =
{
    {"pi", curv::make_shared<Builtin_Value>(pi)},
    {"tau", curv::make_shared<Builtin_Value>(two_pi)},
    {"inf", curv::make_shared<Builtin_Value>(INFINITY)},
    {"null", curv::make_shared<Builtin_Value>(Value())},
    {"false", curv::make_shared<Builtin_Value>(Value(false))},
    {"true", curv::make_shared<Builtin_Value>(Value(true))},
    {"sqrt", curv::make_shared<Builtin_Value>(make_ref_value<Function>(builtin_sqrt, 1))},
    {"len", curv::make_shared<Builtin_Value>(make_ref_value<Function>(builtin_len, 1))},
    {"file", curv::make_shared<Builtin_Value>(make_ref_value<Function>(builtin_file, 1))},
};
